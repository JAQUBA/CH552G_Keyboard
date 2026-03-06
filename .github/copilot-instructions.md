# CH552G Keyboard — Copilot Instructions

## Project

Firmware for a 3-key HID keyboard with rotary encoder and NeoPixel backlighting,
based on the **WCH CH552G** microcontroller (MCS51 / 8051 architecture).  
The project contains two PlatformIO environments:
- **board** — CH552G firmware (SDCC, C language)
- **app** — Windows configurator application (MinGW GCC, C++17, JQB_WindowsLib library)

Built with **PlatformIO** using a custom platform ([JQB_CH55XPlatform](https://github.com/JAQUBA/JQB_CH55XPlatform)) and the **ch55xduino** framework.

---

## Architecture & Constraints

| Parameter | Value |
|---|---|
| MCU | WCH CH552G (MCS51 / 8051) |
| Clock | 24 MHz |
| Flash | 14 336 B (16 KB − 2 KB bootloader) |
| XRAM | 876 B (1 KB − 148 B USB endpoints) |
| Internal RAM | 256 B |
| DataFlash (EEPROM) | 128 B |
| Compiler | **SDCC** (Small Device C Compiler) |
| Firmware language | **C only** — SDCC for MCS51 **does not support C++** |
| Memory model | `--model-large --int-long-reent` |

### Critical Rules (firmware)

- **Do not use C++** — no classes, templates, `new`, `delete`, namespaces, or references `&`.
- Firmware source files must have the **`.c`** extension (not `.cpp`).
- Memory is extremely limited — avoid large buffers and recursion.
- Global variables default to XRAM (`__xdata`). For time-critical data, use `__data` (internal RAM, max 256 B).
- **Do not use `printf` / `sprintf`** — too large for 8051. Use manual conversion.
- **USB Serial (`USBSerial_*`) is unavailable** — CDC is disabled entirely (`-DUSER_USB_RAM=148`).
- When multiplying 8-bit values, SDCC promotes to `int` (16-bit signed) — **cast to `(uint16_t)`** to avoid overflow (e.g. `((uint16_t)s * remainder) >> 8`).

---

## Composite USB HID

The firmware operates as a **composite USB HID** device with two interfaces:

| Interface | Function | Endpoint | Descriptor |
|---|---|---|---|
| Interface 0 | HID Keyboard + Consumer Control | EP1 IN (16 B) | Keyboard (Report ID 1) + Consumer (Report ID 2) |
| Interface 1 | HID Vendor (config) | EP0 (control) | Feature Reports |

- **VID/PID:** `0x1209` / `0xC55D`
- `-DUSER_USB_RAM=148` disables the default CDC and enables the custom USB stack.
- Implementation in `src/board/userUsbHidKeyboard/`:
  - `USBconstant.c/h` — USB descriptors, VID/PID, Report Descriptors for both interfaces
  - `USBhandler.c/h` — USB interrupt handler (EP0 SETUP/IN/OUT, EP1 IN, EP2 IN)
  - `USBHIDKeyboard.c/h` — Keyboard API: `Keyboard_press()`, `Keyboard_release()`, `Keyboard_write()`
- Initialization: `USBInit()` in `setup()`.

### Feature Reports (Interface 1 — Vendor)

Keyboard configuration is done via **HID Feature Reports** on EP0 (control transfers).

> **IMPORTANT:** In the DATA phase of control transfers (GET_REPORT / SET_REPORT), the **first byte is the Report ID**. Data starts at `Ep0Buffer[1]`, not `Ep0Buffer[0]`.

| Report ID | Data size | Direction | Description |
|---|---|---|---|
| 1 | 1 B | IN (EP2) | Dummy Input Report (required by Windows) |
| 2 | 7 B | GET/SET Feature | Key mapping (mod+key) |
| 3 | 7 B | GET/SET Feature | Encoder mapping (mod+key) |
| 4 | 1 B | SET Feature | Command (0x42 = bootloader) |
| 5 | 7 B | GET/SET Feature | LED configuration |
| 6 | 7 B | GET/SET Feature | Long-press key mapping |
| 7 | 7 B | GET/SET Feature | Long-press encoder button |

#### Report ID 2 — Key Mapping (7 data bytes):

| Offset | Field | Description |
|---|---|---|
| 0 | `key1_mod` | Key 1 modifier bitmask |
| 1 | `key1_code` | Key 1 keycode |
| 2 | `key2_mod` | Key 2 modifier bitmask |
| 3 | `key2_code` | Key 2 keycode |
| 4 | `key3_mod` | Key 3 modifier bitmask |
| 5 | `key3_code` | Key 3 keycode |
| 6 | `enc_btn_mod` | Encoder button modifier bitmask |

#### Report ID 3 — Encoder Mapping (7 data bytes):

| Offset | Field | Description |
|---|---|---|
| 0 | `enc_btn_code` | Encoder button keycode |
| 1 | `enc_cw_mod` | CW rotation modifier bitmask |
| 2 | `enc_cw_key` | CW rotation keycode |
| 3 | `enc_ccw_mod` | CCW rotation modifier bitmask |
| 4 | `enc_ccw_key` | CCW rotation keycode |
| 5 | (reserved) | 0 |
| 6 | (reserved) | 0 |

#### Report ID 4 — Command (1 data byte):

| Value | Action |
|---|---|
| `0x42` (`CMD_BOOTLOADER`) | Enter USB bootloader |

#### Report ID 5 — LED Configuration (7 data bytes):

| Offset | Field | Description |
|---|---|---|
| 0 | `led_brightness` | LED brightness (0–255) |
| 1 | `led_mode` | LED mode (0=rainbow, 1=static, 2=breathe) |
| 2 | `led_r` | Red (0–255) |
| 3 | `led_g` | Green (0–255) |
| 4 | `led_b` | Blue (0–255) |
| 5 | `led_toggle` | Toggle mask (bit0=key1, bit1=key2, bit2=key3) |
| 6 | (reserved) | 0 |

#### Report ID 6 — Long-Press Key Mapping (7 data bytes):

| Offset | Field | Description |
|---|---|---|
| 0 | `key1_long_mod` | Key 1 long-press modifier bitmask |
| 1 | `key1_long_code` | Key 1 long-press keycode |
| 2 | `key2_long_mod` | Key 2 long-press modifier bitmask |
| 3 | `key2_long_code` | Key 2 long-press keycode |
| 4 | `key3_long_mod` | Key 3 long-press modifier bitmask |
| 5 | `key3_long_code` | Key 3 long-press keycode |
| 6 | `enc_btn_long_mod` | Encoder button long-press modifier bitmask |

#### Report ID 7 — Long-Press Encoder Button (7 data bytes):

| Offset | Field | Description |
|---|---|---|
| 0 | `enc_btn_long_code` | Encoder button long-press keycode |
| 1–6 | (reserved) | 0 |

### Long Press Behavior

When a long-press keycode is configured (non-zero), the button uses timer-based detection:
- **Short tap** (< 500 ms): fires the normal key on release (press + immediate release)
- **Long hold** (≥ 500 ms): fires the long-press key when threshold is reached; releases on button up
- When long-press is **disabled** (code = 0): button works as before (press on down, release on up)

### Multimedia (Consumer Control) Keys

Keys with codes 0x01–0x07 are **Consumer Control** keys sent via USB Report ID 2 (Consumer Control):

| Code | Constant | USB Usage | Description |
|---|---|---|---|
| 0x01 | `KC_MEDIA_PLAY_PAUSE` | 0x00CD | Play/Pause |
| 0x02 | `KC_MEDIA_STOP` | 0x00B7 | Stop |
| 0x03 | `KC_MEDIA_PREV_TRACK` | 0x00B6 | Previous Track |
| 0x04 | `KC_MEDIA_NEXT_TRACK` | 0x00B5 | Next Track |
| 0x05 | `KC_MEDIA_VOL_UP` | 0x00E9 | Volume Up |
| 0x06 | `KC_MEDIA_VOL_DOWN` | 0x00EA | Volume Down |
| 0x07 | `KC_MEDIA_MUTE` | 0x00E2 | Mute |

Use `IS_MEDIA_KEY(k)` macro to check if a key code is a multimedia key.

### Modifier Bitmask

Each key and encoder function has an associated **modifier bitmask** (`uint8_t`). Bits correspond to the HID Keyboard modifier byte:

| Bit | Constant | Modifier |
|-----|----------|----------|
| 0 | `MOD_BIT_LCTRL` (0x01) | Left Ctrl |
| 1 | `MOD_BIT_LSHIFT` (0x02) | Left Shift |
| 2 | `MOD_BIT_LALT` (0x04) | Left Alt |
| 3 | `MOD_BIT_LGUI` (0x08) | Left GUI (Win) |
| 4 | `MOD_BIT_RCTRL` (0x10) | Right Ctrl |
| 5 | `MOD_BIT_RSHIFT` (0x20) | Right Shift |
| 6 | `MOD_BIT_RALT` (0x40) | Right Alt |
| 7 | `MOD_BIT_RGUI` (0x80) | Right GUI (Win) |

Example: Ctrl+Alt+F6 → `mod = MOD_BIT_LCTRL | MOD_BIT_LALT` (0x05), `key = 0xC3` (F6).

The firmware sends modifiers via `press_mod_bits()` / `release_mod_bits()`, which iterate bits 0–7 and call `Keyboard_press(0x80 + i)` / `Keyboard_release(0x80 + i)`.

### Vendor HID Usage (Interface 1)

```
Usage Page: 0xFF00 (Vendor Defined)
Usage:      0x01
```

The Windows app connects to Interface 1 using these Usage values.

---

## Configuration — EEPROM (DataFlash)

Key mapping, encoder mapping, and LED settings are **not hardcoded in `#define`** — they are stored in **DataFlash (EEPROM)** on the CH552 and loaded at startup.

### Struct `KeyboardConfig` (`config.h`)

```c
typedef struct {
    uint8_t key1_mod;        // Key 1 modifier bitmask
    uint8_t key1_code;       // Key 1 keycode
    uint8_t key2_mod;        // Key 2 modifier bitmask
    uint8_t key2_code;       // Key 2 keycode
    uint8_t key3_mod;        // Key 3 modifier bitmask
    uint8_t key3_code;       // Key 3 keycode
    uint8_t enc_btn_mod;     // Encoder button modifier bitmask
    uint8_t enc_btn_code;    // Encoder button keycode
    uint8_t enc_cw_mod;      // CW modifier bitmask
    uint8_t enc_cw_key;      // CW keycode
    uint8_t enc_ccw_mod;     // CCW modifier bitmask
    uint8_t enc_ccw_key;     // CCW keycode
    uint8_t led_brightness;  // Brightness (0–255)
    uint8_t led_mode;        // LED mode (0/1/2)
    uint8_t led_r, led_g, led_b; // Static color
    uint8_t led_toggle;      // Per-key toggle mask
    uint8_t key1_long_mod;   // Key 1 long-press modifier
    uint8_t key1_long_code;  // Key 1 long-press keycode (0=disabled)
    uint8_t key2_long_mod;   // Key 2 long-press modifier
    uint8_t key2_long_code;  // Key 2 long-press keycode (0=disabled)
    uint8_t key3_long_mod;   // Key 3 long-press modifier
    uint8_t key3_long_code;  // Key 3 long-press keycode (0=disabled)
    uint8_t enc_btn_long_mod;  // Encoder long-press modifier
    uint8_t enc_btn_long_code; // Encoder long-press keycode (0=disabled)
} KeyboardConfig;

extern __xdata KeyboardConfig g_config;
```

### API (`config.c`)

| Function | Description |
|---|---|
| `config_load()` | Load from EEPROM; if magic byte missing → `config_reset()` |
| `config_save()` | Save to EEPROM (26 bytes + magic) |
| `config_reset()` | Restore defaults and save |
| `config_pack_report2(buf)` | Pack g_config → Report ID 2 buffer (7 B) — key mapping |
| `config_unpack_report2(buf)` | Unpack Report ID 2 buffer → g_config |
| `config_pack_report3(buf)` | Pack g_config → Report ID 3 buffer (7 B) — encoder mapping |
| `config_unpack_report3(buf)` | Unpack Report ID 3 buffer → g_config |
| `config_pack_report5(buf)` | Pack g_config → Report ID 5 buffer (7 B) — LED config |
| `config_unpack_report5(buf)` | Unpack Report ID 5 buffer → g_config |
| `config_pack_report6(buf)` | Pack g_config → Report ID 6 buffer (7 B) — long-press keys |
| `config_unpack_report6(buf)` | Unpack Report ID 6 buffer → g_config |
| `config_pack_report7(buf)` | Pack g_config → Report ID 7 buffer (7 B) — long-press encoder |
| `config_unpack_report7(buf)` | Unpack Report ID 7 buffer → g_config |

### EEPROM Layout (DataFlash)

| Address | Field | Default value |
|---|---|---|
| 0 | Magic (`0xA7`) | — |
| 1 | key1_mod | `0x00` (none) |
| 2 | key1_code | `0x62` ('b') |
| 3 | key2_mod | `0x00` (none) |
| 4 | key2_code | `0x66` ('f') |
| 5 | key3_mod | `0x00` (none) |
| 6 | key3_code | `0x65` ('e') |
| 7 | enc_btn_mod | `0x00` (none) |
| 8 | enc_btn_code | `0xB0` (Enter) |
| 9 | enc_cw_mod | `0x02` (LShift bitmask) |
| 10 | enc_cw_key | `0xDA` (Up) |
| 11 | enc_ccw_mod | `0x02` (LShift bitmask) |
| 12 | enc_ccw_key | `0xD9` (Down) |
| 13 | led_brightness | `40` |
| 14 | led_mode | `0` (rainbow) |
| 15 | led_r | `255` |
| 16 | led_g | `255` |
| 17 | led_b | `255` |
| 18 | led_toggle | `0x00` |
| 19 | key1_long_mod | `0x00` (none) |
| 20 | key1_long_code | `0x00` (disabled) |
| 21 | key2_long_mod | `0x00` (none) |
| 22 | key2_long_code | `0x00` (disabled) |
| 23 | key3_long_mod | `0x00` (none) |
| 24 | key3_long_code | `0x00` (disabled) |
| 25 | enc_btn_long_mod | `0x00` (none) |
| 26 | enc_btn_long_code | `0x00` (disabled) |

> **EEPROM_CONFIG_SIZE = 27.** Magic was changed from 0xA6 to 0xA7 when long-press and multimedia support were added — devices with older firmware will automatically reset configuration to defaults on first boot.

### Shared Protocol — `src/shared/protocol.h`

The `protocol.h` file is included by both firmware (C) and the app (C++). It defines:
- VID/PID, Usage Page/Usage
- Report IDs and sizes (`REPORT_ID_KEYS=2`, `REPORT_ID_ENCODER=3`, `REPORT_ID_COMMAND=4`, `REPORT_ID_LED=5`, `CONFIG_REPORT_SIZE=7`)
- Modifier bitmask: `MOD_BIT_LCTRL` (0x01) – `MOD_BIT_RGUI` (0x80)
- EEPROM layout (addresses and magic byte 0xA7)
- LED modes (`LED_MODE_RAINBOW=0`, `LED_MODE_STATIC=1`, `LED_MODE_BREATHE=2`)
- Default values with modifiers (`DEFAULT_KEY1_MOD`, `DEFAULT_KEY1`, `DEFAULT_CW_MOD`, etc.)
- Key codes (`KC_LEFT_SHIFT=0x81`, `KC_UP_ARROW=0xDA`, etc.) — duplicate of `USBHIDKeyboard.h` for the app

---

## Pin Mapping — Keyboard Hardware

ch55xduino uses the scheme: **`PortNumber * 10 + PinNumber`**

Pins are fixed (determined by PCB) and defined as `#define` in `main.c`:

| Function | MCU Pin | Code Number | Configuration |
|---|---|---|---|
| Key 1 | P1.1 | `11` | `INPUT_PULLUP`, active LOW |
| Key 2 | P1.7 | `17` | `INPUT_PULLUP`, active LOW |
| Key 3 | P1.6 | `16` | `INPUT_PULLUP`, active LOW |
| Encoder Channel A | P3.0 | `30` | `INPUT_PULLUP` |
| Encoder Channel B | P3.1 | `31` | `INPUT_PULLUP` |
| Encoder Button | P3.3 | `33` | `INPUT_PULLUP`, active LOW |
| NeoPixel (3× WS2812) | P3.4 | `34` | `OUTPUT`, GRB format |

### All CH552G Pins

| MCU Pin | Code Number | Notes |
|---|---|---|
| P1.0 | `10` | GPIO |
| P1.1 | `11` | GPIO / analog ch0 |
| P1.4 | `14` | GPIO / analog ch1 |
| P1.5 | `15` | GPIO / analog ch2 / PWM1 |
| P1.6 | `16` | GPIO |
| P1.7 | `17` | GPIO |
| P3.0 | `30` | GPIO / PWM1_ |
| P3.1 | `31` | GPIO / PWM2_ |
| P3.2 | `32` | GPIO / analog ch3 |
| P3.3 | `33` | GPIO |
| P3.4 | `34` | GPIO / PWM2 |

> **Warning:** P3.6 and P3.7 are USB lines (D+ / D−) — **never** use as GPIO!

---

## NeoPixel (WS2812) & LED Modes

- 3 WS2812 LEDs on pin P3.4, **GRB** format.
- Library: `WS2812` from ch55xduino (included via `build_src_filter` in `platformio.ini`).
- Buffer: `__xdata uint8_t led_data[NUM_LEDS * 3]` (9 bytes XRAM).
- Animation throttled by `led_frame_cnt` — not every `loop()` iteration updates the LED state.

### Functions

- `set_pixel_for_GRB_LED(buffer, index, r, g, b)` — set pixel color
- `neopixel_show_P3_4(buffer, length)` — send data to LEDs (dedicated to pin P3.4)
- `hsv_to_rgb(h, s, v, &r, &g, &b)` — HSV → RGB conversion (used by rainbow mode)

### LED Modes (`led_mode` in config)

| Value | Name | Description |
|---|---|---|
| 0 | Rainbow | Rotating HSV gradient. Brightness from `led_brightness`. Ignores RGB color. |
| 1 | Static | Fixed color `(led_r, led_g, led_b)` scaled by `led_brightness`. |
| 2 | Breathe | Pulsing color — `(led_r, led_g, led_b)` with rising/falling brightness up to `led_brightness`. |

### LED Toggle Overlay

Each of the 3 keys can have toggle mode enabled (bit in `led_toggle`):
- On key press → flip LED state (`led_toggle_state ^= bit`)
- If LED is "off" → pixel is black (overrides mode calculation)
- Toggle works independently of the selected LED mode

---

## Bootloader

The `enterBootloader()` function in `main.c` switches the MCU to ROM bootloader mode (address `0x3800`):

```c
void enterBootloader(void) {
    EA = 0;                    // disable all interrupts
    USB_INT_EN = 0;            // disable USB interrupts
    USB_CTRL   = 0;            // disable USB controller
    UDEV_CTRL  = 0;            // release D+ pull-up → host sees disconnect
    delayMicroseconds(50000);  // 4× ~50 ms = ~200 ms — enough for host
    delayMicroseconds(50000);
    delayMicroseconds(50000);
    delayMicroseconds(50000);
    __asm__("ljmp 0x3800\n");  // jump to ROM bootloader
    while (1);
}
```

**Critical:** `UDEV_CTRL = 0` is required so the host (Windows) detects the HID device disconnect before re-enumeration in bootloader mode. Without it, the host still sees the device as a connected keyboard.

The bootloader is triggered remotely from the Windows app via Feature Report ID 4 with command `0x42`.

---

## Windows Configurator (`src/app/`)

Desktop C++17 application using **JQB_WindowsLib** (`lib_deps` in `platformio.ini`).
The app uses a **dark UI theme** (Catppuccin Mocha-inspired) with owner-draw buttons and custom color palette.

### Source Files

| File | Description |
|---|---|
| `main.cpp` | Main module: UI layout, HID communication, live LED preview, auto-save |
| `theme.h` | Dark color palette constants (CLR_BG, CLR_SURFACE, CLR_TEXT, CLR_ACCENT, etc.) |
| `keymap.h` / `keymap.cpp` | VK ↔ firmware key code conversions + modifier bitmask helpers |
| `keycapture.h` / `keycapture.cpp` | Owner-draw key-capture button widget with subclass-based combo recording |

### Architecture

- Arduino-like model: `init()` / `setup()` / `loop()`
- Communication via **HID Feature Reports** (HID class from JQB_WindowsLib)
- Auto-connect: `tryConnect()` — automatically tries to connect to the keyboard before each operation
- Live LED preview — `loop()` polls changes in LED controls and sends Report 5 in real-time
- Auto-save on close — `onClose()` callback writes config to device and closes HID (prevents `loop()` from overwriting with stale values after window destruction)

### UI — Dark Theme with 3 Tabs

All UI text is in **English**. The theme is defined in `theme.h` with Catppuccin Mocha-inspired colors.

| Tab | Contents |
|---|---|
| **Keys** | Owner-draw combo-capture buttons (click → press a key combo, e.g. Ctrl+Alt+F6) for 3 keys + encoder button + encoder CW/CCW. Modifiers captured automatically. |
| **LED** | Mode select (Rainbow/Static/Breathe), brightness (0–255 in steps of 10), color (predefined palette). |
| **LED Toggle** | 3 checkboxes — enable/disable per-key LED toggle. |

### Action Buttons (below TabControl)

| Button | Action |
|---|---|
| **Read** | GET Feature Reports 2+3+5 → populate UI |
| **Write** | UI → SET Feature Reports 2+3+5 → EEPROM |
| **Defaults** | Restore defaults in UI (does not write to device) |
| **Bootloader** | SET Feature Report 4 with `CMD_BOOTLOADER` → MCU enters bootloader |

### Key-capture / Combo-capture

Owner-draw buttons on the "Keys" tab use `SetWindowSubclass()` with custom `KeyCaptureProc`:
1. Click → button shows "..." and waits for a key combo
2. `WM_KEYDOWN` of a modifier (Ctrl/Shift/Alt/Win) → preview "LCtrl+LAlt+..." — capture **continues waiting**
3. `WM_KEYDOWN` of a non-modifier → `getAsyncModBits()` + `vkToFwKey()` → **commits combo** (e.g. "LCtrl+F6")
4. `WM_KEYUP` — if all keys released and only a modifier was pressed → **commits modifier** as key (e.g. "LShift")
5. Distinguishes left/right Shift/Ctrl/Alt via `MapVirtualKeyW()` / extended bit
6. `fwKeyToModBit()` converts modifier key code to MOD_BIT_* bit
7. Focus loss → cancels capture

The `KeyCapture` struct stores:
- `uint8_t mod` (bitmask) + `uint8_t code` (key code) — committed result
- `uint8_t pendingMod` — modifier preview during capture
- `uint8_t lastModKey` — last pressed modifier key (for standalone commit)

### HID — Keyboard Connection

```cpp
hid.init();
hid.setVidPid(KB_VID, KB_PID);         // 0x1209, 0xC55D
hid.setUsage(VENDOR_USAGE_PAGE, VENDOR_USAGE);  // 0xFF00, 0x01
hid.setFeatureReportSize(CONFIG_REPORT_SIZE);    // 7
hid.findAndOpen();
```

### Key Code Conversions (in `keymap.h` / `keymap.cpp`)

- `vkToFwKey(UINT vk)` — Windows VK → firmware key code (ASCII / `KC_*` from `protocol.h`)
- `fwKeyToName(uint8_t code)` — firmware key code → display name (e.g. `0xDA` → "Up")
- `fwKeyToModBit(uint8_t code)` — firmware key code → corresponding MOD_BIT_* bit (0 if not a modifier)
- `modBitsToPrefix(uint8_t mod)` — modifier bitmask → string prefix (e.g. "LCtrl+LAlt+")
- `comboToName(uint8_t mod, uint8_t code)` — full combo name (e.g. "LCtrl+LAlt+F6")
- `getAsyncModBits()` — reads currently pressed modifiers via `GetAsyncKeyState()` as a bitmask

---

## Available API (ch55xduino) — Firmware

```c
#include <Arduino.h>

// GPIO
void pinMode(uint8_t pin, uint8_t mode);       // INPUT, OUTPUT, INPUT_PULLUP
void digitalWrite(uint8_t pin, uint8_t val);    // HIGH, LOW
uint8_t digitalRead(uint8_t pin);

// Analog
uint16_t analogRead(uint8_t pin);   // pins: 11, 14, 15, 32
void analogWrite(uint8_t pin, uint16_t val); // PWM: 15, 30, 31, 34

// Timing
void delay(uint32_t ms);
void delayMicroseconds(uint16_t us);
uint32_t millis(void);

// EEPROM (DataFlash, 128 B)
void eeprom_write_byte(__data uint8_t addr, __xdata uint8_t val);
uint8_t eeprom_read_byte(__data uint8_t addr);

// USB HID Keyboard (active mode — requires -DUSER_USB_RAM=148)
void USBInit(void);
void Keyboard_press(uint8_t key);
void Keyboard_release(uint8_t key);
void Keyboard_write(uint8_t key);
void Keyboard_releaseAll(void);
// Constants: KEY_LEFT_SHIFT, KEY_UP_ARROW, KEY_DOWN_ARROW, KEY_RETURN, etc.
```

> **USB Serial CDC (`USBSerial_*`) is unavailable** — disabled by `-DUSER_USB_RAM=148`.

---

## Project Structure

```
platformio.ini              # PlatformIO config (env:board + env:app)
resources.rc                # Windows app resources (icon)
src/
  board/
    main.c                  # Firmware: composite HID + encoder + NeoPixel + bootloader
    config.c                # Configuration: EEPROM load/save/reset + pack/unpack
    config.h                # KeyboardConfig struct + API declarations
    userUsbHidKeyboard/     # Custom composite USB HID implementation
      USBconstant.c/h       #   USB descriptors (2 interfaces, Feature Reports)
      USBhandler.c/h        #   USB interrupt handler (EP0 SETUP/IN/OUT, EP1/EP2 IN)
      USBHIDKeyboard.c/h    #   Keyboard API (press/release/write)
  app/
    main.cpp                # Windows configurator: UI, HID communication, live preview
    theme.h                 # Dark color palette (Catppuccin Mocha-inspired)
    keymap.h / keymap.cpp   # VK ↔ firmware key code conversions + modifier helpers
    keycapture.h / .cpp     # Owner-draw key-capture button widget
  shared/
    protocol.h              # Shared protocol: Report IDs, EEPROM layout, key codes
platform/ch552/
  platform.json             # PlatformIO platform manifest
  platform.py               # PlatformIO platform class
  sdcc_compat.h             # SDCC keyword stubs for IntelliSense
  setup.ps1                 # Toolchain install script (SDCC + ch55xduino)
  boards/
    ch552g.json             # CH552G board definition
  builder/
    main.py                 # SCons build script (SDCC) + IntelliSense auto-gen
    frameworks/
      arduino.py            # ch55xduino integration
.vscode/
  c_cpp_properties.json     # Auto-generated by builder (do not edit manually!)
```

---

## Building & Uploading

```bash
# Build firmware (auto-regenerates .vscode/c_cpp_properties.json)
pio run -e board

# Upload firmware (USB bootloader — connect CH552G in bootloader mode)
pio run -e board -t upload

# Build Windows configurator
pio run -e app

# Run Windows configurator
pio run -e app -t upload

# First-time setup — install toolchain (SDCC + ch55xduino + vnproch55x)
powershell -ExecutionPolicy Bypass -File platform\ch552\setup.ps1
```

### Upload Notes

- Tool: **vnproch55x** (in `tool-ch55xtools`).
- CH552G must be in bootloader mode (hold the boot button while plugging in USB, or send the bootloader command from the app).
- The builder includes a wrapper that ignores verification errors (`"Packet N doesn't match"` — firmware is flashed correctly, verification may fail on some bootloader versions).
- IHX → BIN conversion is handled automatically by the builder.
- Alternative programmer: `ch55xtool` (Python/libusb) — `pip install ch55xtool`.

---

## IntelliSense

The builder automatically generates `.vscode/c_cpp_properties.json` on every `pio run`. The file `platform/ch552/sdcc_compat.h` is set as `forcedInclude` — it maps SDCC keywords (`__xdata`, `__sfr`, `__at()`, `__sbit`, etc.) to standard C so IntelliSense doesn't report errors.

**Do not edit** `.vscode/c_cpp_properties.json` manually — it will be overwritten on the next build.

---

## Copilot Guidelines

### Firmware (`env:board`)

1. **Always generate C code**, never C++. Extension `.c`.
2. Mind memory constraints — 876 B XRAM, 14 KB Flash, 128 B EEPROM.
3. USB pins (P3.6 = 36, P3.7 = 37) are reserved — do not configure them as GPIO.
4. Firmware runs as **composite HID** — `USBSerial_*()` is **unavailable**.
5. For key handling: `Keyboard_press()` / `Keyboard_release()` from `USBHIDKeyboard.h`.
6. Key configuration is in `g_config` (EEPROM-backed) — **not in `#define`**.
7. For GET_REPORT: `Ep0Buffer[0] = Report ID`, data from `Ep0Buffer[1]`, `len = 1 + CONFIG_REPORT_SIZE`.
8. For SET_REPORT (EP0_OUT): data from `Ep0Buffer[1]` (skip Report ID at [0]).
9. Debouncing — polling in `loop()` with `SCAN_DELAY_MS` (5 ms).
10. Encoder — edge detection on Channel A, read B for direction.
11. NeoPixel: `set_pixel_for_GRB_LED()` + `neopixel_show_P3_4()`. Buffer in XRAM.
12. **Avoid `printf` / `sprintf`** — too large for 8051.
13. 8-bit multiplication: always cast to `(uint16_t)` — SDCC int = 16-bit signed, easy to overflow.
14. Bootloader: `UDEV_CTRL = 0` required so host detects USB disconnect before re-enumeration.

### Windows App (`env:app`)

15. Uses `platform = native` and `JQB_WindowsLib` (GitHub) as `lib_deps`.
16. C++17, UNICODE, static linking flags — added automatically by the library.
17. Model: `init()` / `setup()` / `loop()` — global functions, not in a class.
18. HID: `hid.init()` → `hid.setVidPid()` → `hid.setUsage()` → `hid.setFeatureReportSize()` → `hid.findAndOpen()`.
19. `tryConnect()` — auto-connect helper called before Read/Write/Bootloader.
20. Live LED preview — `loop()` polls Select/CheckBox changes and sends Report 5.
21. Auto-save on close — `onClose()` calls `writeToDevice()` then `hid.close()` to prevent `loop()` from overwriting with stale values.
22. Key-capture — owner-draw buttons with `SetWindowSubclass()` + `WM_KEYDOWN` → `vkToFwKey()` + `getAsyncModBits()` → combo (mod bitmask + key code).
23. Dark theme — Catppuccin Mocha-inspired palette in `theme.h`. Owner-draw buttons with hover effects.
24. **Shared code:** `src/shared/protocol.h` — included by both environments (C and C++).
