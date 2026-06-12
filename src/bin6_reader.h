/* -----------------------------------------------------------------------------------------
          BIN6 Protocol Reader for Shahe Linear Scales

  Replaces the synchronous caliper reader with an interrupt-based BIN6 decoder.

  BIN6 protocol (24-bit packet, LSB-first):
    bits 0-19:  position data (20-bit unsigned)
    bit 20:     sign (0=positive, 1=negative)
    bit 21:     always 1 (start bit, used for validation)
    bit 22:     always 0
    bit 23:     units (0=mm, 1=inches)
    Resolution: 10um per count -> mm = raw / 100, inches = raw / 2000

  Hardware: 74HC14 inverting schmitt trigger level shifter
    - Interrupt on FALLING edge (inverted rising edge = data valid)
    - Data bits inverted by hardware
*/

#ifndef BIN6_READER_H
#define BIN6_READER_H

#include <Arduino.h>

#define BIN6_DATA_INVERT  true
#define BIN6_CLK_MODE     FALLING

struct bin6_state_t {
  volatile uint32_t packet;
  volatile uint8_t  bit_count;
  volatile bool     ready;
  volatile int32_t  position_raw;
};

// Two BIN6 channels (mapped to calipers[0]=X, calipers[2]=Z)
static bin6_state_t bin6_states[2] = {};

static void IRAM_ATTR bin6_isr_X() {
  uint32_t p = bin6_states[0].packet;
  uint8_t  bc = bin6_states[0].bit_count;

  if (digitalRead(calipers[0].dataPIN) ^ BIN6_DATA_INVERT)
    p |= (1UL << bc);
  bc++;

  if (bc == 24) {
    if (p & (1UL << 21)) {
      int32_t raw = p & 0xFFFFF;
      if (p & (1UL << 20)) raw = -raw;
      bin6_states[0].position_raw = raw;
      bin6_states[0].ready = true;
    }
    bc = 0;
    p = 0;
  }

  bin6_states[0].packet    = p;
  bin6_states[0].bit_count = bc;
}

static void IRAM_ATTR bin6_isr_Z() {
  uint32_t p = bin6_states[1].packet;
  uint8_t  bc = bin6_states[1].bit_count;

  if (digitalRead(calipers[2].dataPIN) ^ BIN6_DATA_INVERT)
    p |= (1UL << bc);
  bc++;

  if (bc == 24) {
    if (p & (1UL << 21)) {
      int32_t raw = p & 0xFFFFF;
      if (p & (1UL << 20)) raw = -raw;
      bin6_states[1].position_raw = raw;
      bin6_states[1].ready = true;
    }
    bc = 0;
    p = 0;
  }

  bin6_states[1].packet    = p;
  bin6_states[1].bit_count = bc;
}

void bin6_init() {
  // X axis (calipers[0]: CLK=27, DATA=22)
  pinMode(calipers[0].clockPIN, INPUT);
  pinMode(calipers[0].dataPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(calipers[0].clockPIN), bin6_isr_X, BIN6_CLK_MODE);

  // Z axis (calipers[2]: CLK=35, DATA=5)
  pinMode(calipers[2].clockPIN, INPUT);
  pinMode(calipers[2].dataPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(calipers[2].clockPIN), bin6_isr_Z, BIN6_CLK_MODE);
}

float readCaliper(int caliperNumber) {
  if (caliperNumber == 1 || !calipers[caliperNumber].enabled) {
    calipers[caliperNumber].error = 1;
    return 0.0;
  }

  int idx = (caliperNumber == 2) ? 1 : 0;
  bin6_state_t *s = &bin6_states[idx];

  if (s->ready) {
    float result = (float)s->position_raw / 100.0f;
    if (calipers[caliperNumber].direction) result = -result;
    s->ready = false;
    calipers[caliperNumber].error = 0;
    return result;
  }

  if (calipers[caliperNumber].lastReadTime == 0) {
    calipers[caliperNumber].error = 1;
    return 0.0;
  }

  calipers[caliperNumber].error = 0;
  return calipers[caliperNumber].reading;
}

#ifdef SIMULATE_SCALES

// generates fake BIN6 position data so the UI can be seen in Wokwi
static int32_t sim_pos[2] = {0};
static int32_t sim_dir[2] = {1, -1};

void simulate_scales() {
  static unsigned long lastSim = 0;
  unsigned long now = millis();
  if (now - lastSim < 100) return;
  lastSim = now;

  for (int i = 0; i < 2; i++) {
    sim_pos[i] += sim_dir[i] * 8;
    if (sim_pos[i] > 30000 || sim_pos[i] < -30000) sim_dir[i] = -sim_dir[i];
    bin6_states[i].position_raw = sim_pos[i];
    bin6_states[i].ready = true;
    bin6_states[i].bit_count = 0;
    bin6_states[i].packet = 0;
  }
}
#endif

#endif
