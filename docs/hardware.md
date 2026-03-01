# Hardware Reference

## The Keyboard

This project targets a cheap 3-key mechanical macro keyboard with rotary encoder and RGB LEDs, widely available on AliExpress:

**[RGB 3-Key Macro Mechanical Keyboard with Knob](https://www.aliexpress.com/item/1005006308067425.html)**

<p align="center">
  <img src="images/keyboard_front.jpg" width="400" alt="Keyboard front view" />
  <img src="images/keyboard_pcb.jpg" width="400" alt="Keyboard PCB" />
</p>

| Parameter | Value |
|---|---|
| MCU | **WCH CH552G** (MCS51 / 8051 core, 24 MHz) |
| Flash | 14 KB (16 KB minus 2 KB bootloader) |
| XRAM | 876 B usable (1 KB minus 148 B reserved for USB endpoints) |
| Internal RAM | 256 B |
| DataFlash (EEPROM) | 128 B |
| Keys | 3 × hot-swappable mechanical switches |
| Encoder | Rotary encoder with push button |
| LEDs | 3 × WS2812 (NeoPixel), GRB format |
| Connector | USB Type-C |
| Case | Black or white plastic, translucent diffuser |

## Pinout

The pin mapping is fixed by the PCB design and **must not be changed**:

| Function | MCU Pin | Code Number | Configuration |
|---|---|---|---|
| Key 1 | P1.1 | `11` | INPUT_PULLUP, active LOW |
| Key 2 | P1.7 | `17` | INPUT_PULLUP, active LOW |
| Key 3 | P1.6 | `16` | INPUT_PULLUP, active LOW |
| Encoder Channel A | P3.0 | `30` | INPUT_PULLUP |
| Encoder Channel B | P3.1 | `31` | INPUT_PULLUP |
| Encoder Button | P3.3 | `33` | INPUT_PULLUP, active LOW |
| NeoPixel Data (3× WS2812) | P3.4 | `34` | OUTPUT, GRB format |

> **Warning:** P3.6 and P3.7 are USB D+/D− lines. **Never** configure them as GPIO.

### Full CH552G Pin Map

| MCU Pin | Code # | Notes |
|---|---|---|
| P1.0 | `10` | GPIO |
| P1.1 | `11` | GPIO / ADC ch0 — **Key 1** |
| P1.4 | `14` | GPIO / ADC ch1 |
| P1.5 | `15` | GPIO / ADC ch2 / PWM1 |
| P1.6 | `16` | GPIO — **Key 3** |
| P1.7 | `17` | GPIO — **Key 2** |
| P3.0 | `30` | GPIO / PWM1_ — **Encoder A** |
| P3.1 | `31` | GPIO / PWM2_ — **Encoder B** |
| P3.2 | `32` | GPIO / ADC ch3 |
| P3.3 | `33` | GPIO — **Encoder Button** |
| P3.4 | `34` | GPIO / PWM2 — **NeoPixel** |
| P3.6 | — | USB D+ (**do not use**) |
| P3.7 | — | USB D− (**do not use**) |

## Photos

> **Add your own photos!** Place images in the `docs/images/` directory:
> - `keyboard_front.jpg` — Front view of the assembled keyboard
> - `keyboard_pcb.jpg` — PCB / internals
> - `keyboard_side.jpg` — Side view showing encoder and USB-C
> - `app_screenshot.png` — Windows configurator screenshot

The keyboard looks like this (product images from AliExpress):

<p align="center">
  <img src="images/aliexpress_1.jpg" width="350" alt="AliExpress product photo 1" />
  <img src="images/aliexpress_2.jpg" width="350" alt="AliExpress product photo 2" />
</p>

## LED Details

- 3 × WS2812 addressable RGB LEDs mounted under each key
- Data pin: P3.4, GRB color order
- Driven by the ch55xduino `WS2812` library
- Buffer uses 9 bytes of XRAM (`3 LEDs × 3 bytes`)

## Mechanical Switches

- Hot-swappable Cherry MX-compatible sockets
- Any 3-pin or 5-pin mechanical switch works
- Default keycaps included (blank)
