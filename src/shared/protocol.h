/*
 * protocol.h — Shared protocol definitions for CH552G Keyboard
 *
 * Used by both the firmware (C / SDCC) and the Windows configurator (C++).
 * Keep this file pure-C compatible (no C++ features).
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

/* ------------------------------------------------------------------ */
/*  USB identifiers                                                    */
/* ------------------------------------------------------------------ */
#define KB_VID  0x1209
#define KB_PID  0xC55D

/* Vendor HID usage (Interface 1) */
#define VENDOR_USAGE_PAGE  0xFF00
#define VENDOR_USAGE       0x01

/* ------------------------------------------------------------------ */
/*  HID Feature Report IDs  (Vendor interface, Interface 1)            */
/* ------------------------------------------------------------------ */
#define REPORT_ID_VENDOR_IN  1   /* dummy 1-byte Input report for EP2 */
#define REPORT_ID_KEYS       2   /* key mapping         (7 bytes)     */
#define REPORT_ID_ENCODER    3   /* encoder mapping     (7 bytes)     */
#define REPORT_ID_COMMAND    4   /* command report      (1 byte)      */
#define REPORT_ID_LED        5   /* LED configuration   (7 bytes)     */
#define REPORT_ID_LONG_KEYS  6   /* long-press key mapping (7 bytes)  */
#define REPORT_ID_LONG_ENC   7   /* long-press encoder     (7 bytes)  */

#define CONFIG_REPORT_SIZE   7   /* data bytes per Feature report     */
#define CMD_REPORT_SIZE      1   /* data bytes for command report     */

/* Command codes (Feature Report ID 4, byte 0) */
#define CMD_BOOTLOADER       0x42  /* enter USB bootloader              */

/*
 *  Feature Report ID 2 — key mapping (7 data bytes):
 *    [0]  Key1 modifier bitmask
 *    [1]  Key1 code
 *    [2]  Key2 modifier bitmask
 *    [3]  Key2 code
 *    [4]  Key3 modifier bitmask
 *    [5]  Key3 code
 *    [6]  Encoder button modifier bitmask
 *
 *  Feature Report ID 3 — encoder mapping (7 data bytes):
 *    [0]  Encoder button code
 *    [1]  Encoder CW modifier bitmask
 *    [2]  Encoder CW key
 *    [3]  Encoder CCW modifier bitmask
 *    [4]  Encoder CCW key
 *    [5]  (reserved, 0)
 *    [6]  (reserved, 0)
 *
 *  Feature Report ID 5 — LED configuration (7 data bytes):
 *    [0]  LED brightness  (0-255)
 *    [1]  LED mode         (0=rainbow, 1=static, 2=breathe)
 *    [2]  LED static Red   (0-255)
 *    [3]  LED static Green (0-255)
 *    [4]  LED static Blue  (0-255)
 *    [5]  LED toggle mask  (bit0=key1, bit1=key2, bit2=key3)
 *    [6]  (reserved, 0)
 *
 *  Feature Report ID 6 — long-press key mapping (7 data bytes):
 *    [0]  Key1 long-press modifier bitmask
 *    [1]  Key1 long-press code
 *    [2]  Key2 long-press modifier bitmask
 *    [3]  Key2 long-press code
 *    [4]  Key3 long-press modifier bitmask
 *    [5]  Key3 long-press code
 *    [6]  Encoder button long-press modifier bitmask
 *
 *  Feature Report ID 7 — long-press encoder extended (7 data bytes):
 *    [0]  Encoder button long-press code
 *    [1-6]  (reserved, 0)
 */

/* ------------------------------------------------------------------ */
/*  Modifier bitmask (maps to HID keyboard modifier byte bits)         */
/* ------------------------------------------------------------------ */
#define MOD_BIT_LCTRL   0x01
#define MOD_BIT_LSHIFT  0x02
#define MOD_BIT_LALT    0x04
#define MOD_BIT_LGUI    0x08
#define MOD_BIT_RCTRL   0x10
#define MOD_BIT_RSHIFT  0x20
#define MOD_BIT_RALT    0x40
#define MOD_BIT_RGUI    0x80

/* ------------------------------------------------------------------ */
/*  EEPROM / DataFlash layout  (128 bytes available on CH552)          */
/* ------------------------------------------------------------------ */
#define EEPROM_MAGIC          0xA7  /* bumped from 0xA6 — long press + multimedia */

#define EEPROM_ADDR_MAGIC       0
#define EEPROM_ADDR_KEY1_MOD    1
#define EEPROM_ADDR_KEY1        2
#define EEPROM_ADDR_KEY2_MOD    3
#define EEPROM_ADDR_KEY2        4
#define EEPROM_ADDR_KEY3_MOD    5
#define EEPROM_ADDR_KEY3        6
#define EEPROM_ADDR_ENC_BTN_MOD 7
#define EEPROM_ADDR_ENC_BTN     8
#define EEPROM_ADDR_CW_MOD      9
#define EEPROM_ADDR_CW_KEY     10
#define EEPROM_ADDR_CCW_MOD    11
#define EEPROM_ADDR_CCW_KEY    12
#define EEPROM_ADDR_LED_BRT    13
#define EEPROM_ADDR_LED_MODE   14
#define EEPROM_ADDR_LED_R      15
#define EEPROM_ADDR_LED_G      16
#define EEPROM_ADDR_LED_B      17
#define EEPROM_ADDR_LED_TOGGLE 18
#define EEPROM_ADDR_KEY1_LONG_MOD  19
#define EEPROM_ADDR_KEY1_LONG      20
#define EEPROM_ADDR_KEY2_LONG_MOD  21
#define EEPROM_ADDR_KEY2_LONG      22
#define EEPROM_ADDR_KEY3_LONG_MOD  23
#define EEPROM_ADDR_KEY3_LONG      24
#define EEPROM_ADDR_ENC_BTN_LONG_MOD 25
#define EEPROM_ADDR_ENC_BTN_LONG   26
#define EEPROM_CONFIG_SIZE         27

/* ------------------------------------------------------------------ */
/*  LED modes                                                          */
/* ------------------------------------------------------------------ */
#define LED_MODE_RAINBOW  0
#define LED_MODE_STATIC   1
#define LED_MODE_BREATHE  2

/* ------------------------------------------------------------------ */
/*  Default config values                                              */
/* ------------------------------------------------------------------ */
#define DEFAULT_KEY1_MOD  0x00  /* no modifier         */
#define DEFAULT_KEY1      0x62  /* 'b'                 */
#define DEFAULT_KEY2_MOD  0x00  /* no modifier         */
#define DEFAULT_KEY2      0x66  /* 'f'                 */
#define DEFAULT_KEY3_MOD  0x00  /* no modifier         */
#define DEFAULT_KEY3      0x65  /* 'e'                 */
#define DEFAULT_ENC_BTN_MOD 0x00 /* no modifier        */
#define DEFAULT_ENC_BTN   0xB0  /* KEY_RETURN          */
#define DEFAULT_CW_MOD    MOD_BIT_LSHIFT  /* Left Shift bitmask */
#define DEFAULT_CW_KEY    0xDA  /* KEY_UP_ARROW        */
#define DEFAULT_CCW_MOD   MOD_BIT_LSHIFT  /* Left Shift bitmask */
#define DEFAULT_CCW_KEY   0xD9  /* KEY_DOWN_ARROW      */
#define DEFAULT_LED_BRT   40
#define DEFAULT_LED_MODE  LED_MODE_RAINBOW
#define DEFAULT_LED_R     255
#define DEFAULT_LED_G     255
#define DEFAULT_LED_B     255
#define DEFAULT_LED_TOGGLE 0x00  /* no toggle by default */

/* Long-press defaults (0 = disabled) */
#define DEFAULT_KEY1_LONG_MOD    0x00
#define DEFAULT_KEY1_LONG        0x00
#define DEFAULT_KEY2_LONG_MOD    0x00
#define DEFAULT_KEY2_LONG        0x00
#define DEFAULT_KEY3_LONG_MOD    0x00
#define DEFAULT_KEY3_LONG        0x00
#define DEFAULT_ENC_BTN_LONG_MOD 0x00
#define DEFAULT_ENC_BTN_LONG     0x00

/* Long-press threshold (ms) */
#define LONG_PRESS_MS  500

/* ------------------------------------------------------------------ */
/*  Key code constants  (mirrors USBHIDKeyboard.h)                     */
/*  Duplicated here so the Windows app can use them without the        */
/*  firmware-specific header.                                          */
/* ------------------------------------------------------------------ */
#define KC_LEFT_CTRL   0x80
#define KC_LEFT_SHIFT  0x81
#define KC_LEFT_ALT    0x82
#define KC_LEFT_GUI    0x83
#define KC_RIGHT_CTRL  0x84
#define KC_RIGHT_SHIFT 0x85
#define KC_RIGHT_ALT   0x86
#define KC_RIGHT_GUI   0x87

#define KC_UP_ARROW    0xDA
#define KC_DOWN_ARROW  0xD9
#define KC_LEFT_ARROW  0xD8
#define KC_RIGHT_ARROW 0xD7
#define KC_BACKSPACE   0xB2
#define KC_TAB         0xB3
#define KC_RETURN      0xB0
#define KC_ESC         0xB1
#define KC_INSERT      0xD1
#define KC_DELETE      0xD4
#define KC_PAGE_UP     0xD3
#define KC_PAGE_DOWN   0xD6
#define KC_HOME        0xD2
#define KC_END         0xD5
#define KC_CAPS_LOCK   0xC1
#define KC_F1          0xC2
#define KC_F2          0xC3
#define KC_F3          0xC4
#define KC_F4          0xC5
#define KC_F5          0xC6
#define KC_F6          0xC7
#define KC_F7          0xC8
#define KC_F8          0xC9
#define KC_F9          0xCA
#define KC_F10         0xCB
#define KC_F11         0xCC
#define KC_F12         0xCD

/* ------------------------------------------------------------------ */
/*  Multimedia (Consumer Control) key codes                            */
/*  Range 0x01–0x07 — unused by standard keyboard code space.          */
/*  The firmware sends these via the Consumer Control HID report.      */
/* ------------------------------------------------------------------ */
#define KC_MEDIA_PLAY_PAUSE  0x01
#define KC_MEDIA_STOP        0x02
#define KC_MEDIA_PREV_TRACK  0x03
#define KC_MEDIA_NEXT_TRACK  0x04
#define KC_MEDIA_VOL_UP      0x05
#define KC_MEDIA_VOL_DOWN    0x06
#define KC_MEDIA_MUTE        0x07

#define KC_MEDIA_FIRST  KC_MEDIA_PLAY_PAUSE
#define KC_MEDIA_LAST   KC_MEDIA_MUTE
#define IS_MEDIA_KEY(k) ((k) >= KC_MEDIA_FIRST && (k) <= KC_MEDIA_LAST)

#endif /* PROTOCOL_H */
