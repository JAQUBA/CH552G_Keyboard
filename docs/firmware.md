# Firmware Documentation

## Overview

The firmware runs on the **WCH CH552G** microcontroller (MCS51/8051 architecture) and implements a **composite USB HID device** with two interfaces:

| Interface | Function | Endpoint |
|---|---|---|
| Interface 0 | HID Keyboard (Boot protocol, 6KRO) | EP1 IN (8 bytes) |
| Interface 1 | HID Vendor (Configuration) | EP0 (Control transfers) |

The firmware is written in **pure C** (SDCC compiler — C++ is not supported on MCS51).

## Source Files

| File | Description |
|---|---|
| [src/board/main.c](../src/board/main.c) | Main firmware: GPIO setup, key scanning, encoder handling, NeoPixel animation, bootloader |
| [src/board/config.c](../src/board/config.c) | EEPROM configuration: load/save/reset, report pack/unpack |
| [src/board/config.h](../src/board/config.h) | `KeyboardConfig` struct and API declarations |
| [src/board/userUsbHidKeyboard/](../src/board/userUsbHidKeyboard/) | Custom composite USB HID stack |
| [src/shared/protocol.h](../src/shared/protocol.h) | Shared protocol definitions (used by both firmware and app) |

## How It Works

### Startup (`setup()`)

1. Load configuration from DataFlash (EEPROM) via `config_load()`
2. Initialize USB composite device via `USBInit()`
3. Configure GPIO pins (keys, encoder, NeoPixel)

### Main Loop (`loop()`)

The main loop runs every 5 ms (`SCAN_DELAY_MS`) and performs:

1. **Bootloader check** — if host sent `CMD_BOOTLOADER` (0x42), jump to ROM bootloader
2. **Button scanning** — read 3 keys + encoder button, debounce, send HID keypresses with modifier support
3. **Encoder reading** — detect edges on channel A, read channel B for direction, send programmed keystrokes
4. **LED animation** — update NeoPixel colors based on current mode (rainbow/static/breathe)
5. **LED toggle overlay** — blank LEDs that are toggled off by key presses

### Key Handling

Each key has two configurable parameters stored in EEPROM:
- **Modifier bitmask** (`uint8_t`) — which modifiers to hold (Ctrl, Shift, Alt, GUI)
- **Key code** (`uint8_t`) — the actual key to send

When a key is pressed:
1. `press_mod_bits()` iterates the modifier bitmask and calls `Keyboard_press(0x80+i)` for each set bit
2. `Keyboard_press(key_code)` sends the actual key
3. On release, the reverse happens

### Encoder

- Edge detection on Channel A (falling edge)
- Channel B state determines direction (CW vs CCW)
- Each step sends a configurable key combo (mod+key) as a brief pulse (30 ms)

### LED Modes

| Mode | Value | Description |
|---|---|---|
| Rainbow | 0 | Rotating HSV gradient across all 3 LEDs. Uses `led_brightness` for value. |
| Static | 1 | Fixed color `(led_r, led_g, led_b)` scaled by `led_brightness`. |
| Breathe | 2 | Pulsing brightness with configured color. Fades between 0 and `led_brightness`. |

**LED Toggle:** Each key can independently toggle its LED on/off on each press (configured via `led_toggle` bitmask).

### Bootloader Entry

The firmware can enter the CH552's ROM bootloader (at address `0x3800`) for USB firmware updates:

```c
void enterBootloader(void) {
    EA = 0;              // Disable all interrupts
    USB_INT_EN = 0;      // Disable USB interrupts
    USB_CTRL = 0;        // Disable USB controller
    UDEV_CTRL = 0;       // Release D+ pull-up (host sees disconnect)
    // ~250 ms delay for Windows to deregister HID device
    __asm__("ljmp 0x3800\n");  // Jump to ROM bootloader
}
```

> **Critical:** Setting `UDEV_CTRL = 0` is required so the host detects the USB disconnect before re-enumeration in bootloader mode.

## Memory Constraints

| Resource | Total | Used by USB | Available |
|---|---|---|---|
| Flash | 16 KB | 2 KB (bootloader) | **14 KB** |
| XRAM | 1024 B | 148 B (USB endpoints) | **876 B** |
| IRAM | 256 B | — | 256 B |
| DataFlash | 128 B | — | 128 B (19 B used for config) |

### Important SDCC Rules

- **No C++** — SDCC for MCS51 only supports C
- **No `printf`/`sprintf`** — too large for 8051, use manual conversion
- **No USB Serial** — CDC is disabled (`-DUSER_USB_RAM=148`)
- **Cast 8-bit multiplications to `uint16_t`** — SDCC promotes to 16-bit signed `int`
- Variables default to XRAM (`__xdata`). Use `__data` for time-critical data (max 256 B)

## Building

```bash
# Build firmware
pio run -e board

# Upload to CH552G (must be in bootloader mode)
pio run -e board -t upload
```

To enter bootloader mode:
1. **Hardware:** Hold the boot button while plugging in USB, OR
2. **Software:** Send bootloader command from the Windows configurator app
