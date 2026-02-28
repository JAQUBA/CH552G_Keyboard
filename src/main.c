// CH552G Keyboard — HID Keyboard + Encoder + NeoPixel Rainbow
// 3 keys (B/F/E) + rotary encoder (G/H) + encoder button (J)
// 3 NeoPixel LEDs on P3.4

#ifndef USER_USB_RAM
#error "This example needs to be compiled with USER_USB_RAM defined"
#endif

#include <Arduino.h>
#include "userUsbHidKeyboard/USBHIDKeyboard.h"
#include <WS2812.h>

// ============================================================
//  QUICK CONFIG — change key assignments here
// ============================================================

// --- 3 keys with LEDs (accent/modifier keys) ---
#define KEY1_PIN   11          // P1.1 (button B)
#define KEY1_CODE  'b'         // letter to send

#define KEY2_PIN   17          // P1.7 (button F)
#define KEY2_CODE  'f'

#define KEY3_PIN   16          // P1.6 (button E)
#define KEY3_CODE  'e'

// --- Rotary encoder ---
#define ENC_A_PIN  30          // P3.0 (encoder channel A)
#define ENC_B_PIN  31          // P3.1 (encoder channel B)

// What to send on encoder rotation
#define ENC_CW_MOD   KEY_LEFT_SHIFT   // clockwise modifier
#define ENC_CW_KEY   KEY_UP_ARROW     // clockwise key
#define ENC_CCW_MOD  KEY_LEFT_SHIFT   // counter-clockwise modifier
#define ENC_CCW_KEY  KEY_DOWN_ARROW   // counter-clockwise key

// --- Encoder button ---
#define ENC_BTN_PIN  33        // P3.3 (button J)
#define ENC_BTN_CODE KEY_RETURN // what to send on encoder press

// --- NeoPixels ---
#define NEO_PIN    34          // P3.4
#define NUM_LEDS   3
#define LED_BRIGHTNESS 40      // 0-255

// --- Timing ---
#define SCAN_DELAY_MS  5       // main loop delay (ms)
#define ENC_PULSE_MS   30      // how long encoder keystroke lasts (ms)

// ============================================================
//  Internal state
// ============================================================

// NeoPixel buffer (GRB, 3 bytes per LED)
__xdata uint8_t led_data[NUM_LEDS * 3];
static uint8_t hue_offset = 0;

// Button states
static uint8_t key1_prev = 0;
static uint8_t key2_prev = 0;
static uint8_t key3_prev = 0;
static uint8_t enc_btn_prev = 0;

// Encoder state
static uint8_t enc_a_prev = 1;

// ============================================================
//  HSV -> RGB
// ============================================================
void hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v,
                uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        *r = *g = *b = v;
        return;
    }

    region = h / 43;
    remainder = (h - (region * 43)) * 6;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

// ============================================================
//  Helper: handle a simple button (press/release)
// ============================================================
static void handle_button(uint8_t pin, uint8_t key, uint8_t *prev) {
    uint8_t pressed = !digitalRead(pin);
    if (pressed != *prev) {
        *prev = pressed;
        if (pressed) {
            Keyboard_press(key);
        } else {
            Keyboard_release(key);
        }
    }
}

// ============================================================
//  Helper: send encoder pulse (modifier + key, brief press)
// ============================================================
static void encoder_pulse(uint8_t mod, uint8_t key) {
    Keyboard_press(mod);
    Keyboard_press(key);
    delay(ENC_PULSE_MS);
    Keyboard_release(key);
    Keyboard_release(mod);
}

// ============================================================
//  Setup
// ============================================================
void setup() {
    USBInit();

    // Keys with pull-up (active LOW)
    pinMode(KEY1_PIN, INPUT_PULLUP);
    pinMode(KEY2_PIN, INPUT_PULLUP);
    pinMode(KEY3_PIN, INPUT_PULLUP);

    // Encoder channels with pull-up
    pinMode(ENC_A_PIN, INPUT_PULLUP);
    pinMode(ENC_B_PIN, INPUT_PULLUP);

    // Encoder button with pull-up
    pinMode(ENC_BTN_PIN, INPUT_PULLUP);

    // Read initial encoder state
    enc_a_prev = digitalRead(ENC_A_PIN);

    // NeoPixel data pin
    pinMode(NEO_PIN, OUTPUT);
}

// ============================================================
//  Main loop
// ============================================================
void loop() {
    uint8_t i, r, g, b;

    // --- 3 keys ---
    handle_button(KEY1_PIN, KEY1_CODE, &key1_prev);
    handle_button(KEY2_PIN, KEY2_CODE, &key2_prev);
    handle_button(KEY3_PIN, KEY3_CODE, &key3_prev);

    // --- Encoder button ---
    handle_button(ENC_BTN_PIN, ENC_BTN_CODE, &enc_btn_prev);

    // --- Rotary encoder (polling, edge on channel A) ---
    {
        uint8_t enc_a = digitalRead(ENC_A_PIN);
        if (enc_a != enc_a_prev) {
            enc_a_prev = enc_a;
            if (enc_a == 0) {
                // Falling edge on A — check B for direction
                uint8_t enc_b = digitalRead(ENC_B_PIN);
                if (enc_b) {
                    // A fell while B is high → clockwise
                    encoder_pulse(ENC_CW_MOD, ENC_CW_KEY);
                } else {
                    // A fell while B is low → counter-clockwise
                    encoder_pulse(ENC_CCW_MOD, ENC_CCW_KEY);
                }
            }
        }
    }

    // --- Rainbow NeoPixels ---
    for (i = 0; i < NUM_LEDS; i++) {
        uint8_t hue = hue_offset + (i * 85);
        hsv_to_rgb(hue, 255, LED_BRIGHTNESS, &r, &g, &b);
        set_pixel_for_GRB_LED(led_data, i, r, g, b);
    }
    neopixel_show_P3_4(led_data, NUM_LEDS * 3);
    hue_offset += 2;

    delay(SCAN_DELAY_MS);
}
