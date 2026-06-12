# Wiring — 2× Shahe BIN6 Scales → ESP32 CYD DRO

## Pin Assignment

| Signal       | ESP32 GPIO | CYD Location | Notes                  |
|--------------|------------|--------------|------------------------|
| Scale 1 CLK  | 27         | CN1          | X axis                 |
| Scale 1 DATA | 22         | CN1 / P3     |                        |
| Scale 2 CLK  | 35         | P3           | Input-only pin, OK     |
| Scale 2 DATA | 5          | SD card CS   | Sacrifices SD card     |
| 74HC14 VCC   | 3.3V       | CN1          |                        |
| 74HC14 GND   | GND        | CN1          | Common ground          |
| Scale VCC    | 3.3V       | CN1          | Via 1N4148 diode drop  |

## 74HC14 Level Shifter (1 Channel Shown, 4 Needed)

Scale signals are 1.5–3V. The 74HC14 inverting Schmitt trigger cleans and shifts to 3.3V logic. Each channel uses one 74HC14 inverter plus 3 resistors:

```
  Scale CLK/DATA (1.5-3V)
         │
         ├── 4.7kΩ ──┬── 74HC14 input (pin 1A, 3A, 5A, 9A)
         │           │
         │          ┌┴┐
         │          │47kΩ
         │          └┬┘
         │           ┴ GND
         │
         │    ┌──── 10kΩ pull-up to 3.3V
         │    │
        74HC14 output (pin 1Y, 2Y, 3Y, 4Y)
         │
         ├── ESP32 GPIO (27, 22, 35, or 5)
         │
         ┴ GND
```

The 74HC14 inverts the signal. Firmware compensates (FALLING edge interrupt, inverted data bit).

## Full Wiring

```
                     ┌─────────────────────┐
                     │     74HC14          │
                     │  ┌───────────────┐  │
 Scale1 CLK ──┬──────┤──│1A  (pin 1)    │  │
              │      │  │1Y  (pin 2)────├──── GPIO 27 (X CLK)
             ┌┴┐     │  │               │  │
             │47kΩ   │  │2A  (pin 3)    │  │
             └┬┘     │  │2Y  (pin 4)────├──── GPIO 22 (X DATA)
              │      │  │               │  │
 Scale1 DATA ─┤──────┤──│3A  (pin 5)    │  │
              │      │  │3Y  (pin 6)────├──── GPIO 35 (Z CLK)
             ┌┴┐     │  │               │  │
             │47kΩ   │  │4A  (pin 9)    │  │
             └┬┘     │  │4Y  (pin 8)────├──── GPIO 5  (Z DATA)
              │      │  │               │  │
 Scale2 CLK ──┤──────┤──│5A (pin 11)    │  │   (unused)
              │      │  │5Y (pin 10)    │  │
 Scale2 DATA ─┤──────┤──│6A (pin 13)    │  │   (unused)
                     │  │6Y (pin 12)    │  │
                     │  └───────────────┘  │
                     │  VCC=3.3V  GND=GND  │
                     │  0.1µF bypass cap   │
                     └─────────────────────┘
```

Each scale CLK and DATA uses the same resistor network (4.7kΩ + 47kΩ divider, 10kΩ pull-up at 74HC14 input).

## Scale Power

```
  3.3V (CN1) ──┤├── 1N4148 ──┬── Scale1 VCC
               diode         │
                             └── Scale2 VCC
                           ─┴─ 100µF + 0.1µF
                           GND
```

Diode drops ~0.6V → ~2.7V at scale. Decoupling at each scale: 100µF electrolytic + 0.1µF ceramic.

## Notes

- **Shielded twisted-pair** cable for each scale. Connect shield to GND at **ESP32 end only**.
- **74HC14 bypass**: 0.1µF ceramic between VCC and GND as close to the chip as possible.
- **Scale frame**: Connect to machine earth via 100Ω resistor to avoid ground loops.
- **SD card**: Disabled (GPIO5 used for Scale 2 DATA). Remove SD card if inserted.
