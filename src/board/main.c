/*
 * main.c — CH552G Keyboard firmware
 *
 * Composite USB device:
 *   Interface 0 — HID Keyboard  (3 keys + rotary encoder + button)
 *   Interface 1 — HID Vendor    (Feature Reports for config read/write)
 *
 * Key assignments, encoder mapping and LED settings are loaded from
 * DataFlash (EEPROM) on boot.  The Windows configurator app writes
 * new settings via HID Feature Reports — no reboot required.
 */

#ifndef USER_USB_RAM
#error "This firmware needs USER_USB_RAM defined (e.g. 148)"
#endif

#include <Arduino.h>
#include "userUsbHidKeyboard/USBHIDKeyboard.h"
#include "config.h"
#include "protocol.h"
#include <WS2812.h>

/* ============================================================ */
/*  Hardware pin definitions (fixed by PCB, do NOT change)       */
/* ============================================================ */
#define KEY1_PIN    11          /* P1.1  button B   */
#define KEY2_PIN    17          /* P1.7  button F   */
#define KEY3_PIN    16          /* P1.6  button E   */
#define ENC_A_PIN   30          /* P3.0  encoder A  */
#define ENC_B_PIN   31          /* P3.1  encoder B  */
#define ENC_BTN_PIN 33          /* P3.3  encoder SW */
#define NEO_PIN     34          /* P3.4  NeoPixel   */

#define NUM_LEDS       3
#define SCAN_DELAY_MS  5
#define ENC_PULSE_MS   30

/* ============================================================ */
/*  Runtime state                                                */
/* ============================================================ */
__xdata uint8_t led_data[NUM_LEDS * 3];
static uint8_t hue_offset   = 0;
static uint8_t breathe_val   = 0;
static  int8_t breathe_dir   = 1;
static uint8_t led_frame_cnt = 0;   /* throttle LED animation speed */

static uint8_t key1_prev    = 0;
static uint8_t key2_prev    = 0;
static uint8_t key3_prev    = 0;
static uint8_t enc_btn_prev = 0;
static uint8_t enc_a_prev   = 1;
static uint8_t led_toggle_state = 0; /* per-LED on/off state for toggle mode */

/* ============================================================ */
/*  HSV → RGB                                                    */
/* ============================================================ */
void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v,
                uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        *r = *g = *b = v;
        return;
    }

    region    = h / 43;
    remainder = (h - (region * 43)) * 6;

    p = ((uint16_t)v * (255 - s)) >> 8;
    q = ((uint16_t)v * (255 - (((uint16_t)s * remainder) >> 8))) >> 8;
    t = ((uint16_t)v * (255 - (((uint16_t)s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

/* ============================================================ */
/*  Button helper                                                */
/* ============================================================ */
static void handle_button(uint8_t pin, uint8_t key, uint8_t *prev, uint8_t led_idx) {
    uint8_t pressed = !digitalRead(pin);
    if (pressed != *prev) {
        *prev = pressed;
        if (pressed) {
            Keyboard_press(key);
            /* If this key has LED toggle enabled, flip the LED state */
            if (g_config.led_toggle & (1 << led_idx)) {
                led_toggle_state ^= (1 << led_idx);
            }
        } else {
            Keyboard_release(key);
        }
    }
}

/* ============================================================ */
/*  Encoder pulse helper                                         */
/* ============================================================ */
static void encoder_pulse(uint8_t mod, uint8_t key) {
    if (mod) Keyboard_press(mod);
    Keyboard_press(key);
    delay(ENC_PULSE_MS);
    Keyboard_release(key);
    if (mod) Keyboard_release(mod);
}

/* ============================================================ */
/*  Enter USB bootloader                                         */
/* ============================================================ */
void enterBootloader(void) {
    USB_INT_EN = 0;            /* disable USB interrupts       */
    USB_CTRL   = 0;            /* detach USB — host sees disconnect */
    EA = 0;                    /* disable all interrupts       */
    TMOD = 0;                  /* stop timers                  */
    delayMicroseconds(50000);  /* ~50 ms — let host notice     */
    delayMicroseconds(50000);  /* ~50 ms more                  */
    __asm__("ljmp 0x3800\n"); /* jump to ROM bootloader       */
    while (1);                 /* should never reach here      */
}

/* ============================================================ */
/*  Setup                                                        */
/* ============================================================ */
void setup() {
    /* Load key/encoder/LED config from DataFlash (EEPROM) */
    config_load();

    USBInit();

    /* Keys — active LOW, internal pull-up */
    pinMode(KEY1_PIN,    INPUT_PULLUP);
    pinMode(KEY2_PIN,    INPUT_PULLUP);
    pinMode(KEY3_PIN,    INPUT_PULLUP);

    /* Encoder channels */
    pinMode(ENC_A_PIN,   INPUT_PULLUP);
    pinMode(ENC_B_PIN,   INPUT_PULLUP);
    pinMode(ENC_BTN_PIN, INPUT_PULLUP);

    enc_a_prev = digitalRead(ENC_A_PIN);

    /* NeoPixel */
    pinMode(NEO_PIN, OUTPUT);
}

/* ============================================================ */
/*  Main loop                                                    */
/* ============================================================ */
void loop() {
    uint8_t i, r, g, b;

    /* --- Buttons (use runtime config) --- */
    handle_button(KEY1_PIN,    g_config.key1_code,    &key1_prev,    0);
    handle_button(KEY2_PIN,    g_config.key2_code,    &key2_prev,    1);
    handle_button(KEY3_PIN,    g_config.key3_code,    &key3_prev,    2);
    handle_button(ENC_BTN_PIN, g_config.enc_btn_code, &enc_btn_prev, 3);

    /* --- Rotary encoder --- */
    {
        uint8_t enc_a = digitalRead(ENC_A_PIN);
        if (enc_a != enc_a_prev) {
            enc_a_prev = enc_a;
            if (enc_a == 0) {
                uint8_t enc_b = digitalRead(ENC_B_PIN);
                if (enc_b) {
                    encoder_pulse(g_config.enc_cw_mod, g_config.enc_cw_key);
                } else {
                    encoder_pulse(g_config.enc_ccw_mod, g_config.enc_ccw_key);
                }
            }
        }
    }

    /* --- NeoPixel --- */
    led_frame_cnt++;

    if (g_config.led_mode == LED_MODE_RAINBOW) {
        /* Rainbow — rotating hue across LEDs */
        for (i = 0; i < NUM_LEDS; i++) {
            uint8_t hue = hue_offset + (i * 85);
            hsv_to_rgb(hue, 255, g_config.led_brightness, &r, &g, &b);
            set_pixel_for_GRB_LED(led_data, i, r, g, b);
        }
        if (led_frame_cnt >= 6) {  /* ~30 ms per step */
            led_frame_cnt = 0;
            hue_offset++;
        }
    } else if (g_config.led_mode == LED_MODE_STATIC) {
        /* Static colour from config (R, G, B scaled by brightness) */
        for (i = 0; i < NUM_LEDS; i++) {
            r = ((uint16_t)g_config.led_r * g_config.led_brightness) >> 8;
            g = ((uint16_t)g_config.led_g * g_config.led_brightness) >> 8;
            b = ((uint16_t)g_config.led_b * g_config.led_brightness) >> 8;
            set_pixel_for_GRB_LED(led_data, i, r, g, b);
        }
    } else if (g_config.led_mode == LED_MODE_BREATHE) {
        /* Breathe — pulsing brightness with configured colour */
        for (i = 0; i < NUM_LEDS; i++) {
            r = ((uint16_t)g_config.led_r * breathe_val) >> 8;
            g = ((uint16_t)g_config.led_g * breathe_val) >> 8;
            b = ((uint16_t)g_config.led_b * breathe_val) >> 8;
            set_pixel_for_GRB_LED(led_data, i, r, g, b);
        }
        if (led_frame_cnt >= 4) {  /* ~20 ms per step */
            int16_t nv;
            led_frame_cnt = 0;
            nv = (int16_t)breathe_val + breathe_dir * 2;
            if (nv >= (int16_t)g_config.led_brightness) {
                breathe_val = g_config.led_brightness;
                breathe_dir = -1;
            } else if (nv <= 0) {
                breathe_val = 0;
                breathe_dir = 1;
            } else {
                breathe_val = (uint8_t)nv;
            }
        }
    } else {
        /* Unknown mode — fallback to rainbow */
        g_config.led_mode = LED_MODE_RAINBOW;
    }

    /* Apply per-LED toggle: if a key has toggle enabled and its LED
       is currently "off", blank that pixel. */
    for (i = 0; i < NUM_LEDS; i++) {
        if ((g_config.led_toggle & (1 << i)) &&
            !(led_toggle_state & (1 << i))) {
            set_pixel_for_GRB_LED(led_data, i, 0, 0, 0);
        }
    }

    neopixel_show_P3_4(led_data, NUM_LEDS * 3);

    delay(SCAN_DELAY_MS);
}
