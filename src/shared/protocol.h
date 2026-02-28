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
#define REPORT_ID_CONFIG1    2   /* key / encoder config (7 bytes)    */
#define REPORT_ID_CONFIG2    3   /* LED / extras config  (7 bytes)    */
#define REPORT_ID_COMMAND    4   /* command report (1 byte)           */

#define CONFIG_REPORT_SIZE   7   /* data bytes per Feature report     */
#define CMD_REPORT_SIZE      1   /* data bytes for command report     */

/* Command codes (Feature Report ID 4, byte 0) */
#define CMD_BOOTLOADER       0x42  /* enter USB bootloader              */

/*
 *  Feature Report ID 2 layout (7 data bytes):
 *    [0]  Key1 code
 *    [1]  Key2 code
 *    [2]  Key3 code
 *    [3]  Encoder button code
 *    [4]  Encoder CW modifier
 *    [5]  Encoder CW key
 *    [6]  Encoder CCW modifier
 *
 *  Feature Report ID 3 layout (7 data bytes):
 *    [0]  Encoder CCW key
 *    [1]  LED brightness  (0-255)
 *    [2]  LED mode         (0=rainbow, 1=static, 2=breathe)
 *    [3..6]  reserved (0)
 */

/* ------------------------------------------------------------------ */
/*  EEPROM / DataFlash layout  (128 bytes available on CH552)          */
/* ------------------------------------------------------------------ */
#define EEPROM_MAGIC          0xA5

#define EEPROM_ADDR_MAGIC     0
#define EEPROM_ADDR_KEY1      1
#define EEPROM_ADDR_KEY2      2
#define EEPROM_ADDR_KEY3      3
#define EEPROM_ADDR_ENC_BTN   4
#define EEPROM_ADDR_CW_MOD    5
#define EEPROM_ADDR_CW_KEY    6
#define EEPROM_ADDR_CCW_MOD   7
#define EEPROM_ADDR_CCW_KEY   8
#define EEPROM_ADDR_LED_BRT   9
#define EEPROM_ADDR_LED_MODE  10
#define EEPROM_CONFIG_SIZE    11

/* ------------------------------------------------------------------ */
/*  LED modes                                                          */
/* ------------------------------------------------------------------ */
#define LED_MODE_RAINBOW  0
#define LED_MODE_STATIC   1
#define LED_MODE_BREATHE  2

/* ------------------------------------------------------------------ */
/*  Default config values                                              */
/* ------------------------------------------------------------------ */
#define DEFAULT_KEY1      0x62  /* 'b' */
#define DEFAULT_KEY2      0x66  /* 'f' */
#define DEFAULT_KEY3      0x65  /* 'e' */
#define DEFAULT_ENC_BTN   0xB0  /* KEY_RETURN      */
#define DEFAULT_CW_MOD    0x81  /* KEY_LEFT_SHIFT  */
#define DEFAULT_CW_KEY    0xDA  /* KEY_UP_ARROW    */
#define DEFAULT_CCW_MOD   0x81  /* KEY_LEFT_SHIFT  */
#define DEFAULT_CCW_KEY   0xD9  /* KEY_DOWN_ARROW  */
#define DEFAULT_LED_BRT   40
#define DEFAULT_LED_MODE  LED_MODE_RAINBOW

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

#endif /* PROTOCOL_H */
