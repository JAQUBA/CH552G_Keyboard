# Windows Configurator App

## Overview

The Windows desktop application allows you to configure the CH552G Keyboard over USB without reflashing firmware. It communicates with the keyboard using **HID Feature Reports** on the Vendor interface.

Built with **C++17** using [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib) — a native WinAPI GUI library with an Arduino-like programming model.

<p align="center">
  <img src="images/app_keys_tab.png" width="450" alt="Keys tab" />
</p>

## Features

- **Auto-connect** — Automatically finds and connects to the keyboard
- **Key combo capture** — Click a button, press any key combination (e.g. Ctrl+Alt+F6), and it's captured
- **Live LED preview** — LED changes are sent to the keyboard in real-time as you adjust settings
- **EEPROM save** — Write configuration to persist across power cycles
- **Remote bootloader** — Enter firmware update mode without physical button

## User Interface

The app has a tabbed layout with 3 tabs and a row of action buttons:

### Tab 1: Keys

| Control | Description |
|---|---|
| Key 1 / Key 2 / Key 3 | Click → press a key combo → captured (e.g. "LCtrl+F6") |
| Encoder Button | Click → press key combo for encoder push |
| Encoder CW/CCW | Click → press key combo for clockwise/counter-clockwise rotation |

**How key capture works:**
1. Click any key button — it shows `"..."`
2. Press modifier keys (Ctrl/Shift/Alt/Win) — preview shows `"LCtrl+LAlt+..."`
3. Press a non-modifier key — combo is committed (e.g. `"LCtrl+LAlt+F6"`)
4. Or release all keys after pressing only modifiers — modifier becomes the key (e.g. `"LShift"`)
5. Click away to cancel

### Tab 2: LED

| Control | Options |
|---|---|
| Mode | Rainbow, Static, Breathe |
| Brightness | 0–255 (in steps of 10) |
| Color | White, Red, Green, Blue, Yellow, Cyan, Magenta, Orange, Purple |

> Color only affects Static and Breathe modes. Rainbow ignores the color setting.

### Tab 3: LED Toggle

| Control | Description |
|---|---|
| Key 1 Toggle | Pressing key 1 toggles its LED on/off |
| Key 2 Toggle | Pressing key 2 toggles its LED on/off |
| Key 3 Toggle | Pressing key 3 toggles its LED on/off |

### Action Buttons

| Button | Action |
|---|---|
| **Read** | Read current config from keyboard → populate UI |
| **Write** | Send UI settings to keyboard → save to EEPROM |
| **Defaults** | Reset UI to factory defaults (does not write to device) |
| **Bootloader** | Send bootloader command → keyboard enters USB bootloader mode |

## Building

```bash
# Build the Windows app
pio run -e app

# Build and run
pio run -e app -t upload
```

The build system automatically handles:
- C++17 standard
- UNICODE defines
- Static linking (no runtime DLL dependencies)
- Windows subsystem (no console window)
- Resource compilation (icon)

## HID Connection

The app connects to the keyboard using:
- VID/PID: `0x1209` / `0xC55D`
- Usage Page: `0xFF00` (Vendor Defined)
- Usage: `0x01`
- Feature Report Size: 7 bytes

```cpp
hid.init();
hid.setVidPid(KB_VID, KB_PID);
hid.setUsage(VENDOR_USAGE_PAGE, VENDOR_USAGE);
hid.setFeatureReportSize(CONFIG_REPORT_SIZE);
hid.findAndOpen();
```

## Source

The entire app is in a single file: [src/app/main.cpp](../src/app/main.cpp)

Key functions:
- `vkToFwKey()` — Convert Windows Virtual Key to firmware key code
- `fwKeyToName()` — Convert firmware key code to display name
- `fwKeyToModBit()` — Convert modifier key code to bitmask bit
- `getAsyncModBits()` — Read currently pressed modifiers
- `comboToName()` — Full display name for a key combo (e.g. "LCtrl+Alt+F6")
- `readFromDevice()` — GET Feature Reports and populate UI
- `writeToDevice()` — Send UI values as SET Feature Reports
- `sendLedConfig()` — Live-send LED settings (Report 5 only)
