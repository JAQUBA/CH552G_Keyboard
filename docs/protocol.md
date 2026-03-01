# USB HID Protocol

## Overview

The CH552G Keyboard operates as a **composite USB HID device** with two interfaces:

| Interface | Type | Endpoint | Purpose |
|---|---|---|---|
| 0 | HID Keyboard (Boot) | EP1 IN (8 B) | Standard keyboard input (6KRO) |
| 1 | HID Vendor | EP0 (Control) | Configuration via Feature Reports |

**USB Identifiers:**
- VID: `0x1209` (pid.codes open-source)
- PID: `0xC55D`
- Vendor Usage Page: `0xFF00`
- Vendor Usage: `0x01`

## Feature Reports

Configuration is exchanged via **HID Feature Reports** on Interface 1 using control transfers (EP0).

> **Important:** In the DATA phase of control transfers (GET_REPORT / SET_REPORT), the **first byte is the Report ID**. Actual data starts at offset 1 (`Ep0Buffer[1]`).

### Report ID 1 — Dummy Input Report (1 byte)

Required by Windows for Interface 1. Sent on EP2 IN. Not used for configuration.

### Report ID 2 — Key Mapping (7 data bytes)

| Offset | Field | Description |
|---|---|---|
| 0 | `key1_mod` | Key 1 modifier bitmask |
| 1 | `key1_code` | Key 1 keycode |
| 2 | `key2_mod` | Key 2 modifier bitmask |
| 3 | `key2_code` | Key 2 keycode |
| 4 | `key3_mod` | Key 3 modifier bitmask |
| 5 | `key3_code` | Key 3 keycode |
| 6 | `enc_btn_mod` | Encoder button modifier bitmask |

### Report ID 3 — Encoder Mapping (7 data bytes)

| Offset | Field | Description |
|---|---|---|
| 0 | `enc_btn_code` | Encoder button keycode |
| 1 | `enc_cw_mod` | Encoder CW rotation modifier bitmask |
| 2 | `enc_cw_key` | Encoder CW rotation keycode |
| 3 | `enc_ccw_mod` | Encoder CCW rotation modifier bitmask |
| 4 | `enc_ccw_key` | Encoder CCW rotation keycode |
| 5 | _(reserved)_ | 0 |
| 6 | _(reserved)_ | 0 |

### Report ID 4 — Command (1 data byte)

| Value | Action |
|---|---|
| `0x42` | Enter USB bootloader |

### Report ID 5 — LED Configuration (7 data bytes)

| Offset | Field | Description |
|---|---|---|
| 0 | `led_brightness` | Brightness (0–255) |
| 1 | `led_mode` | Mode: 0=Rainbow, 1=Static, 2=Breathe |
| 2 | `led_r` | Static color Red (0–255) |
| 3 | `led_g` | Static color Green (0–255) |
| 4 | `led_b` | Static color Blue (0–255) |
| 5 | `led_toggle` | Toggle bitmask (bit0=key1, bit1=key2, bit2=key3) |
| 6 | _(reserved)_ | 0 |

## Modifier Bitmask

Each key and encoder function has an associated **modifier bitmask** (`uint8_t`). The bits map directly to the HID keyboard modifier byte:

| Bit | Constant | Modifier | Value |
|---|---|---|---|
| 0 | `MOD_BIT_LCTRL` | Left Ctrl | 0x01 |
| 1 | `MOD_BIT_LSHIFT` | Left Shift | 0x02 |
| 2 | `MOD_BIT_LALT` | Left Alt | 0x04 |
| 3 | `MOD_BIT_LGUI` | Left GUI (Win) | 0x08 |
| 4 | `MOD_BIT_RCTRL` | Right Ctrl | 0x10 |
| 5 | `MOD_BIT_RSHIFT` | Right Shift | 0x20 |
| 6 | `MOD_BIT_RALT` | Right Alt | 0x40 |
| 7 | `MOD_BIT_RGUI` | Right GUI (Win) | 0x80 |

**Example:** Ctrl+Alt+F6 → `mod = 0x05` (LCTRL | LALT), `key = 0xC7` (F6)

## EEPROM Layout

The keyboard configuration is stored in CH552's DataFlash (128 bytes). Layout:

| Address | Field | Default |
|---|---|---|
| 0 | Magic byte (`0xA6`) | — |
| 1 | key1_mod | `0x00` |
| 2 | key1_code | `0x62` ('b') |
| 3 | key2_mod | `0x00` |
| 4 | key2_code | `0x66` ('f') |
| 5 | key3_mod | `0x00` |
| 6 | key3_code | `0x65` ('e') |
| 7 | enc_btn_mod | `0x00` |
| 8 | enc_btn_code | `0xB0` (Enter) |
| 9 | enc_cw_mod | `0x02` (LShift) |
| 10 | enc_cw_key | `0xDA` (Up Arrow) |
| 11 | enc_ccw_mod | `0x02` (LShift) |
| 12 | enc_ccw_key | `0xD9` (Down Arrow) |
| 13 | led_brightness | `40` |
| 14 | led_mode | `0` (Rainbow) |
| 15 | led_r | `255` |
| 16 | led_g | `255` |
| 17 | led_b | `255` |
| 18 | led_toggle | `0x00` |

**Total config size: 19 bytes** (1 magic + 18 data)

> The magic byte was changed from `0xA5` to `0xA6` when modifier support was added. Devices with older firmware will automatically reset to defaults on first boot.

## Key Codes

Key codes used in reports (subset — full list in `protocol.h`):

| Code | Key | Code | Key |
|---|---|---|---|
| `0x61`–`0x7A` | a–z | `0x30`–`0x39` | 0–9 |
| `0x20` | Space | `0xB0` | Enter |
| `0xB1` | Escape | `0xB2` | Backspace |
| `0xB3` | Tab | `0xC1` | Caps Lock |
| `0xC2`–`0xCD` | F1–F12 | `0xD1` | Insert |
| `0xD2` | Home | `0xD3` | Page Up |
| `0xD4` | Delete | `0xD5` | End |
| `0xD6` | Page Down | `0xD7` | Right Arrow |
| `0xD8` | Left Arrow | `0xD9` | Down Arrow |
| `0xDA` | Up Arrow | | |
| `0x80` | Left Ctrl | `0x81` | Left Shift |
| `0x82` | Left Alt | `0x83` | Left GUI |
| `0x84` | Right Ctrl | `0x85` | Right Shift |
| `0x86` | Right Alt | `0x87` | Right GUI |

## Communication Flow

### Reading Configuration

```
Host                          Device
  |-- GET_REPORT (ID=2) ------->|
  |<-- 7 bytes key mapping ------|
  |-- GET_REPORT (ID=3) ------->|
  |<-- 7 bytes encoder mapping --|
  |-- GET_REPORT (ID=5) ------->|
  |<-- 7 bytes LED config ------|
```

### Writing Configuration

```
Host                          Device
  |-- SET_REPORT (ID=2, 7B) --->|  Key mapping → g_config
  |-- SET_REPORT (ID=3, 7B) --->|  Encoder mapping → g_config
  |-- SET_REPORT (ID=5, 7B) --->|  LED config → g_config → EEPROM save
```

### Entering Bootloader

```
Host                          Device
  |-- SET_REPORT (ID=4, 1B) --->|  CMD_BOOTLOADER (0x42)
  |                              |  → Sets flag, completes transfer
  |                              |  → main loop: enterBootloader()
  |<-- USB disconnect -----------|
  |-- New enumeration: -------->|  CH552 ROM bootloader (0x3800)
```
