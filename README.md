# Oracle for Misha — Seeed XIAO RP2350

A small gift built around an SSD1306: on boot it scrolls a personal message, then a wire electrode triggers a fortune teller with 120 Belarusian answers.

No Pico SDK, ARM compiler, `picotool`, or serial-port tools needed on the host — everything runs inside Docker.

## Wiring

| Device | XIAO RP2350 |
| --- | --- |
| SSD1306 VCC | 3V3 |
| SSD1306 GND | GND |
| SSD1306 SDA | D4 / GPIO6 |
| SSD1306 SCL | D5 / GPIO7 |
| Electrode wire | D2 / GPIO28 |
| 1–4.7 MΩ resistor | between D1 / GPIO27 and D2 / GPIO28 |

Do not connect the electrode to 5 V. The electrode connects only to D2; D1 is connected to D2 exclusively through the resistor. The firmware measures capacitance charge time, calibrates a background baseline at startup, and filters out accidental touches.

## Build and Flash

Only Docker Engine with USB access is required.

```bash
./run.sh build
```

The first run creates a Docker image with Pico SDK 2.2.0, ARM GCC, `picotool`, and `picocom`, then writes the UF2 to `firmware/build/misha_gadalka.uf2`.

To flash, run the command first — it builds the firmware, then prompts you to enter BOOTSEL mode:

```bash
./run.sh flash
```

When the build finishes, press and hold BOOT on the XIAO RP2350, connect USB, release the button, and press Enter. `picotool` flashes the board from inside the container.

The serial monitor also runs inside the container:

```bash
./run.sh monitor --port /dev/ttyACM0
```

Full cycle (build → flash → monitor): `./run.sh all`.

## Behaviour

- Intro slides scroll upward continuously with no static pauses; a touch during the intro skips straight to the prompt.
- The next touch triggers a spinning animation, then displays a random fortune.
- The deck holds 120 fortunes: 96 "yes" answers and 24 "not now". Answers do not repeat until the full deck is exhausted.
