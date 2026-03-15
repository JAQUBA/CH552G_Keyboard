# CH552G Keyboard

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![PlatformIO](https://img.shields.io/badge/Built%20with-PlatformIO-orange)](https://platformio.org/)
[![CH552G](https://img.shields.io/badge/MCU-WCH%20CH552G-green)](https://www.wch-ic.com/products/CH552.html)

Custom open-source firmware and Windows configurator for a **3-key USB HID macro keyboard** with rotary encoder and NeoPixel RGB backlighting, based on the **WCH CH552G** microcontroller.

<!-- Add your own photos to docs/images/ and uncomment: -->
<!-- <p align="center">
  <img src="docs/images/keyboard_front.jpg" width="500" alt="CH552G Keyboard" />
</p> -->

## Where to Buy

This firmware targets a cheap 3-key macro keyboard widely available on AliExpress (~$5):

**[RGB 3-Key Macro Mechanical Keyboard with Knob — AliExpress](https://www.aliexpress.com/item/1005006308067425.html)**

The keyboard features:
- 3 hot-swappable mechanical switches (Cherry MX compatible)
- Rotary encoder with push button
- 3 × WS2812 RGB LEDs under each key
- USB Type-C connector
- WCH CH552G MCU (MCS51 / 8051 core)

> **Note:** The stock firmware has limited customization. This project replaces it with a fully open-source solution with a dedicated Windows configuration tool.

## Features

- **3 fully programmable keys** — any key or combo (e.g. Ctrl+Alt+F6)
- **Rotary encoder** — programmable button + CW/CCW rotation with modifier support
- **NeoPixel RGB backlighting** (3 × WS2812) — Rainbow, Static color, and Breathe modes
- **Per-key LED toggle** — each key can toggle its LED on/off when pressed
- **USB configuration** — Windows app communicates via HID Feature Reports (no special drivers needed)
- **EEPROM storage** — settings persist across power cycles
- **Remote bootloader** — enter firmware update mode from the configurator app
- **Composite USB HID** — appears as standard keyboard + vendor config interface

## Hardware Specifications

| Parameter | Value |
|---|---|
| MCU | WCH CH552G (MCS51 / 8051, 24 MHz) |
| Flash | 14 KB (16 KB − 2 KB bootloader) |
| XRAM | 876 B (1 KB − 148 B USB endpoints) |
| DataFlash (EEPROM) | 128 B |
| USB | Composite HID (Keyboard + Vendor) |
| VID/PID | `0x1209` / `0xC55D` |
| Keys | 3 × hot-swap mechanical switches |
| Encoder | Rotary encoder with push button |
| LEDs | 3 × WS2812 (NeoPixel), GRB format |
| Connector | USB Type-C |

### Pinout

| Function | Pin | Function | Pin |
|---|---|---|---|
| Key 1 | P1.1 | Encoder A | P3.0 |
| Key 2 | P1.7 | Encoder B | P3.1 |
| Key 3 | P1.6 | Encoder Button | P3.3 |
| | | NeoPixel Data | P3.4 |

## Project Structure

```
CH552G_Keyboard/
├── platformio.ini          PlatformIO config (env:board + env:app)
├── LICENSE                 MIT License
├── app.manifest            Windows Common Controls v6 manifest
├── resources.rc            Windows app resources (icon + manifest)
├── resources/
│   └── icon.ico            Application icon
├── docs/                   Documentation
│   ├── hardware.md           Hardware reference & pinout
│   ├── firmware.md           Firmware architecture & internals
│   ├── configurator.md       Windows app usage & building
│   ├── protocol.md           USB HID protocol & EEPROM layout
│   └── images/               Photos and screenshots
└── src/
    ├── board/              Firmware (C, SDCC compiler)
    │   ├── main.c            Keyboard logic, encoder, NeoPixel, bootloader
    │   ├── config.c/h        EEPROM configuration management
    │   └── userUsbHidKeyboard/  Custom composite USB HID stack
    ├── app/
    │   ├── main.cpp          Windows configurator (C++17, JQB_WindowsLib)
    │   ├── theme.h           Dark color palette (Catppuccin Mocha-inspired)
    │   ├── keymap.h/cpp      VK ↔ firmware key code conversions
    │   └── keycapture.h/cpp  Owner-draw key-capture button widget
    └── shared/
        └── protocol.h       Shared protocol definitions (firmware + app)
```

Two PlatformIO environments:
- **`board`** — CH552G firmware (SDCC, pure C) using [JQB_CH55XPlatform](https://github.com/JAQUBA/JQB_CH55XPlatform)
- **`app`** — Windows configurator (MinGW GCC, C++17) using [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib)

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- All toolchains are downloaded automatically on first build

### Build & Flash Firmware

```bash
# Build firmware
pio run -e board

# Upload (CH552G must be in bootloader mode)
pio run -e board -t upload
```

**Entering bootloader mode:**
- **Hardware:** Hold the boot button on the PCB while plugging in USB
- **Software:** Click "Bootloader" in the configurator app

### Build & Run Configurator

```bash
# Build Windows app
pio run -e app

# Build and launch
pio run -e app -t upload
```

## Windows Configurator

<!-- <p align="center">
  <img src="docs/images/app_keys_tab.png" width="450" alt="Configurator App" />
</p> -->

The configurator connects to the keyboard over USB and provides a tabbed interface:

| Tab | Description |
|---|---|
| **Keys** | Assign key combos: click a button → press your desired combination (e.g. Ctrl+F6) |
| **LED** | Set lighting mode (Rainbow / Static / Breathe), brightness, and color |
| **LED Toggle** | Enable per-key LED toggle (press key to turn its LED on/off) |

| Button | Action |
|---|---|
| **Read** | Read current config from keyboard |
| **Write** | Write settings to keyboard EEPROM |
| **Defaults** | Reset UI to factory defaults |
| **Bootloader** | Enter firmware update mode |

LED changes are sent in **real-time** as you adjust mode, brightness, or color — no need to click Write for preview.

## Documentation

| Document | Description |
|---|---|
| [Hardware Reference](docs/hardware.md) | Board details, pinout, LED and switch info |
| [Firmware Guide](docs/firmware.md) | Firmware architecture, memory constraints, building |
| [Configurator Guide](docs/configurator.md) | Windows app features, key capture, HID connection |
| [USB Protocol](docs/protocol.md) | HID Feature Reports, EEPROM layout, key codes |

## Default Key Mapping

| Input | Default Action |
|---|---|
| Key 1 | `B` |
| Key 2 | `F` |
| Key 3 | `E` |
| Encoder Button | `Enter` |
| Encoder CW | `Shift + Up Arrow` |
| Encoder CCW | `Shift + Down Arrow` |

## Dependencies

| Component | Repository |
|---|---|
| PlatformIO Platform | [JQB_CH55XPlatform](https://github.com/JAQUBA/JQB_CH55XPlatform) |
| Windows GUI Library | [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) |
| Arduino Core | [ch55xduino](https://github.com/DeqingSun/ch55xduino) (auto-downloaded) |
| Compiler | SDCC (auto-downloaded) |
| Upload Tool | vnproch55x (auto-downloaded) |

## Contributing

Contributions are welcome! Feel free to:
- Report bugs or request features via [Issues](../../issues)
- Submit pull requests
- Share your custom keymaps and use cases

## License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [WCH](https://www.wch-ic.com/) for the CH552G microcontroller
- [ch55xduino](https://github.com/DeqingSun/ch55xduino) Arduino core for CH55x
- [pid.codes](https://pid.codes/) for the open-source USB VID `0x1209`
