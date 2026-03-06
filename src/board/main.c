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
#include "userUsbHidKeyboard/USBhandler.h"
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
/*  Media key to USB Consumer usage mapping                      */
/* ============================================================ */
static uint16_t media_fw_to_usage(uint8_t fw_code) {
    switch (fw_code) {
        case KC_MEDIA_PLAY_PAUSE: return 0x00CD;
        case KC_MEDIA_STOP:       return 0x00B7;
        case KC_MEDIA_PREV_TRACK: return 0x00B6;
        case KC_MEDIA_NEXT_TRACK: return 0x00B5;
        case KC_MEDIA_VOL_UP:     return 0x00E9;
        case KC_MEDIA_VOL_DOWN:   return 0x00EA;
        case KC_MEDIA_MUTE:       return 0x00E2;
        default: return 0;
    }
}

/* ============================================================ */
/*  Key action helpers (keyboard or consumer control)             */
/* ============================================================ */
static void press_mod_bits(uint8_t bits);
static void release_mod_bits(uint8_t bits);

static void key_action_press(uint8_t mod, uint8_t key) {
    if (IS_MEDIA_KEY(key)) {
        Consumer_press(media_fw_to_usage(key));
    } else {
        press_mod_bits(mod);
        Keyboard_press(key);
    }
}

static void key_action_release(uint8_t mod, uint8_t key) {
    if (IS_MEDIA_KEY(key)) {
        Consumer_release();
    } else {
        Keyboard_release(key);
        release_mod_bits(mod);
    }
}

/* ============================================================ */
/*  Button state for long-press detection                        */
/* ============================================================ */
typedef struct {
    uint8_t  prev;        /* previous debounced state */
    uint8_t  long_fired;  /* long press already fired this hold */
    uint32_t press_time;  /* millis() when button was pressed */
} ButtonState;

/* ============================================================ */
/*  Runtime state                                                */
/* ============================================================ */
__xdata uint8_t led_data[NUM_LEDS * 3];
static uint8_t hue_offset   = 0;
static uint8_t breathe_val   = 0;
static  int8_t breathe_dir   = 1;
static uint8_t led_frame_cnt = 0;   /* throttle LED animation speed */

static uint8_t enc_a_prev   = 1;
static uint8_t led_toggle_state = 0; /* per-LED on/off state for toggle mode */

static ButtonState bs_key1    = {0, 0, 0};
static ButtonState bs_key2    = {0, 0, 0};
static ButtonState bs_key3    = {0, 0, 0};
static ButtonState bs_enc_btn = {0, 0, 0};

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
/*  Modifier bitmask helpers                                     */
/* ============================================================ */
static void press_mod_bits(uint8_t bits) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (bits & (1 << i)) Keyboard_press(0x80 + i);
    }
}

static void release_mod_bits(uint8_t bits) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (bits & (1 << i)) Keyboard_release(0x80 + i);
    }
}

/* ============================================================ */
/*  Button helper (with long-press support)                      */
/* ============================================================ */
static void handle_button_lp(uint8_t pin,
                             uint8_t mod, uint8_t key,
                             uint8_t long_mod, uint8_t long_key,
                             ButtonState *bs, uint8_t led_idx) {
    uint8_t pressed = !digitalRead(pin);

    if (pressed && !bs->prev) {
        /* Just pressed */
        bs->prev = 1;
        bs->long_fired = 0;
        bs->press_time = millis();

        /* Toggle LED state if enabled */
        if (led_idx < NUM_LEDS && (g_config.led_toggle & (1 << led_idx))) {
            led_toggle_state ^= (1 << led_idx);
        }

        if (long_key == 0) {
            /* No long press configured — immediate press (hold behavior) */
            key_action_press(mod, key);
        }
    } else if (pressed && bs->prev) {
        /* Still held */
        if (long_key != 0 && !bs->long_fired) {
            if ((millis() - bs->press_time) >= LONG_PRESS_MS) {
                bs->long_fired = 1;
                key_action_press(long_mod, long_key);
            }
        }
    } else if (!pressed && bs->prev) {
        /* Just released */
        bs->prev = 0;
        if (long_key == 0) {
            /* No long press — release held key */
            key_action_release(mod, key);
        } else if (bs->long_fired) {
            /* Long press was fired — release it */
            key_action_release(long_mod, long_key);
        } else {
            /* Short tap — press and immediately release */
            key_action_press(mod, key);
            delay(10);
            key_action_release(mod, key);
        }
    }
}

/* ============================================================ */
/*  Encoder pulse helper                                         */
/* ============================================================ */
static void encoder_pulse(uint8_t mod, uint8_t key) {
    key_action_press(mod, key);
    delay(ENC_PULSE_MS);
    key_action_release(mod, key);
}

/* ============================================================ */
/*  Enter USB bootloader                                         */
/* ============================================================ */
void enterBootloader(void) {
    EA = 0;                    /* disable all interrupts         */
    USB_INT_EN = 0;            /* disable USB interrupts         */
    USB_CTRL   = 0;            /* disable USB, release D+ pull-up*/
    UDEV_CTRL  = 0;            /* disable USB port               */
    TMOD = 0;                  /* stop all timers                */
    USB_DEV_AD = 0;            /* reset USB device address       */
    USB_INT_FG = 0xFF;         /* clear pending USB flags        */
    /* ~250 ms — let Windows fully deregister the HID device */
    delayMicroseconds(50000);
    delayMicroseconds(50000);
    delayMicroseconds(50000);
    delayMicroseconds(50000);
    delayMicroseconds(50000);
    __asm__("ljmp 0x3800\n"); /* jump to ROM bootloader         */
    while (1);                 /* should never reach here        */
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

    /* --- Bootloader request (deferred from USB ISR) --- */
    if (bootloader_request) {
        enterBootloader();  /* never returns */
    }

    /* --- Buttons (use runtime config, with long-press support) --- */
    handle_button_lp(KEY1_PIN,
                     g_config.key1_mod, g_config.key1_code,
                     g_config.key1_long_mod, g_config.key1_long_code,
                     &bs_key1, 0);
    handle_button_lp(KEY2_PIN,
                     g_config.key2_mod, g_config.key2_code,
                     g_config.key2_long_mod, g_config.key2_long_code,
                     &bs_key2, 1);
    handle_button_lp(KEY3_PIN,
                     g_config.key3_mod, g_config.key3_code,
                     g_config.key3_long_mod, g_config.key3_long_code,
                     &bs_key3, 2);
    handle_button_lp(ENC_BTN_PIN,
                     g_config.enc_btn_mod, g_config.enc_btn_code,
                     g_config.enc_btn_long_mod, g_config.enc_btn_long_code,
                     &bs_enc_btn, 3);

    /* --- Rotary encoder --- */
    {
        uint8_t enc_a = digitalRead(ENC_A_PIN);
        if (enc_a != enc_a_prev) {
            enc_a_prev = enc_a;
            if (enc_a == 0) {
                uint8_t enc_b = digitalRead(ENC_B_PIN);
                if (enc_b) {
                    encoder_pulse(g_config.enc_ccw_mod, g_config.enc_ccw_key);
                } else {
                    encoder_pulse(g_config.enc_cw_mod, g_config.enc_cw_key);
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
