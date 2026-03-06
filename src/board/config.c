/*
 * config.c — EEPROM-backed keyboard configuration
 */

#include "config.h"

/* protocol.h is in src/shared, found via -I flag */
#include "protocol.h"

/* ch55xduino core provides these (eeprom.c) */
extern void eeprom_write_byte(__data uint8_t addr, __xdata uint8_t val);
extern uint8_t eeprom_read_byte(__data uint8_t addr);

__xdata KeyboardConfig g_config;

/* ---- Load from DataFlash ---- */
void config_load(void) {
    uint8_t magic = eeprom_read_byte(EEPROM_ADDR_MAGIC);
    if (magic != EEPROM_MAGIC) {
        config_reset();
        return;
    }
    g_config.key1_mod       = eeprom_read_byte(EEPROM_ADDR_KEY1_MOD);
    g_config.key1_code      = eeprom_read_byte(EEPROM_ADDR_KEY1);
    g_config.key2_mod       = eeprom_read_byte(EEPROM_ADDR_KEY2_MOD);
    g_config.key2_code      = eeprom_read_byte(EEPROM_ADDR_KEY2);
    g_config.key3_mod       = eeprom_read_byte(EEPROM_ADDR_KEY3_MOD);
    g_config.key3_code      = eeprom_read_byte(EEPROM_ADDR_KEY3);
    g_config.enc_btn_mod    = eeprom_read_byte(EEPROM_ADDR_ENC_BTN_MOD);
    g_config.enc_btn_code   = eeprom_read_byte(EEPROM_ADDR_ENC_BTN);
    g_config.enc_cw_mod     = eeprom_read_byte(EEPROM_ADDR_CW_MOD);
    g_config.enc_cw_key     = eeprom_read_byte(EEPROM_ADDR_CW_KEY);
    g_config.enc_ccw_mod    = eeprom_read_byte(EEPROM_ADDR_CCW_MOD);
    g_config.enc_ccw_key    = eeprom_read_byte(EEPROM_ADDR_CCW_KEY);
    g_config.led_brightness = eeprom_read_byte(EEPROM_ADDR_LED_BRT);
    g_config.led_mode       = eeprom_read_byte(EEPROM_ADDR_LED_MODE);
    g_config.led_r          = eeprom_read_byte(EEPROM_ADDR_LED_R);
    g_config.led_g          = eeprom_read_byte(EEPROM_ADDR_LED_G);
    g_config.led_b          = eeprom_read_byte(EEPROM_ADDR_LED_B);
    g_config.led_toggle     = eeprom_read_byte(EEPROM_ADDR_LED_TOGGLE);
    g_config.key1_long_mod    = eeprom_read_byte(EEPROM_ADDR_KEY1_LONG_MOD);
    g_config.key1_long_code   = eeprom_read_byte(EEPROM_ADDR_KEY1_LONG);
    g_config.key2_long_mod    = eeprom_read_byte(EEPROM_ADDR_KEY2_LONG_MOD);
    g_config.key2_long_code   = eeprom_read_byte(EEPROM_ADDR_KEY2_LONG);
    g_config.key3_long_mod    = eeprom_read_byte(EEPROM_ADDR_KEY3_LONG_MOD);
    g_config.key3_long_code   = eeprom_read_byte(EEPROM_ADDR_KEY3_LONG);
    g_config.enc_btn_long_mod = eeprom_read_byte(EEPROM_ADDR_ENC_BTN_LONG_MOD);
    g_config.enc_btn_long_code= eeprom_read_byte(EEPROM_ADDR_ENC_BTN_LONG);

    /* Validate led_mode — default to rainbow for unknown values */
    if (g_config.led_mode > LED_MODE_BREATHE)
        g_config.led_mode = LED_MODE_RAINBOW;
}

/* Write a single byte only if it differs from the stored value. */
static void eeprom_update_byte(__data uint8_t addr, __xdata uint8_t val) {
    if (eeprom_read_byte(addr) != val)
        eeprom_write_byte(addr, val);
}

/* ---- Save to DataFlash (skip unchanged bytes) ---- */
void config_save(void) {
    eeprom_update_byte(EEPROM_ADDR_KEY1_MOD,   g_config.key1_mod);
    eeprom_update_byte(EEPROM_ADDR_KEY1,       g_config.key1_code);
    eeprom_update_byte(EEPROM_ADDR_KEY2_MOD,   g_config.key2_mod);
    eeprom_update_byte(EEPROM_ADDR_KEY2,       g_config.key2_code);
    eeprom_update_byte(EEPROM_ADDR_KEY3_MOD,   g_config.key3_mod);
    eeprom_update_byte(EEPROM_ADDR_KEY3,       g_config.key3_code);
    eeprom_update_byte(EEPROM_ADDR_ENC_BTN_MOD, g_config.enc_btn_mod);
    eeprom_update_byte(EEPROM_ADDR_ENC_BTN,    g_config.enc_btn_code);
    eeprom_update_byte(EEPROM_ADDR_CW_MOD,     g_config.enc_cw_mod);
    eeprom_update_byte(EEPROM_ADDR_CW_KEY,     g_config.enc_cw_key);
    eeprom_update_byte(EEPROM_ADDR_CCW_MOD,    g_config.enc_ccw_mod);
    eeprom_update_byte(EEPROM_ADDR_CCW_KEY,    g_config.enc_ccw_key);
    eeprom_update_byte(EEPROM_ADDR_LED_BRT,    g_config.led_brightness);
    eeprom_update_byte(EEPROM_ADDR_LED_MODE,   g_config.led_mode);
    eeprom_update_byte(EEPROM_ADDR_LED_R,      g_config.led_r);
    eeprom_update_byte(EEPROM_ADDR_LED_G,      g_config.led_g);
    eeprom_update_byte(EEPROM_ADDR_LED_B,      g_config.led_b);
    eeprom_update_byte(EEPROM_ADDR_LED_TOGGLE, g_config.led_toggle);
    eeprom_update_byte(EEPROM_ADDR_KEY1_LONG_MOD,    g_config.key1_long_mod);
    eeprom_update_byte(EEPROM_ADDR_KEY1_LONG,         g_config.key1_long_code);
    eeprom_update_byte(EEPROM_ADDR_KEY2_LONG_MOD,    g_config.key2_long_mod);
    eeprom_update_byte(EEPROM_ADDR_KEY2_LONG,         g_config.key2_long_code);
    eeprom_update_byte(EEPROM_ADDR_KEY3_LONG_MOD,    g_config.key3_long_mod);
    eeprom_update_byte(EEPROM_ADDR_KEY3_LONG,         g_config.key3_long_code);
    eeprom_update_byte(EEPROM_ADDR_ENC_BTN_LONG_MOD, g_config.enc_btn_long_mod);
    eeprom_update_byte(EEPROM_ADDR_ENC_BTN_LONG,     g_config.enc_btn_long_code);
    eeprom_update_byte(EEPROM_ADDR_MAGIC,      EEPROM_MAGIC);
}

/* ---- Reset to factory defaults ---- */
void config_reset(void) {
    g_config.key1_mod       = DEFAULT_KEY1_MOD;
    g_config.key1_code      = DEFAULT_KEY1;
    g_config.key2_mod       = DEFAULT_KEY2_MOD;
    g_config.key2_code      = DEFAULT_KEY2;
    g_config.key3_mod       = DEFAULT_KEY3_MOD;
    g_config.key3_code      = DEFAULT_KEY3;
    g_config.enc_btn_mod    = DEFAULT_ENC_BTN_MOD;
    g_config.enc_btn_code   = DEFAULT_ENC_BTN;
    g_config.enc_cw_mod     = DEFAULT_CW_MOD;
    g_config.enc_cw_key     = DEFAULT_CW_KEY;
    g_config.enc_ccw_mod    = DEFAULT_CCW_MOD;
    g_config.enc_ccw_key    = DEFAULT_CCW_KEY;
    g_config.led_brightness = DEFAULT_LED_BRT;
    g_config.led_mode       = DEFAULT_LED_MODE;
    g_config.led_r          = DEFAULT_LED_R;
    g_config.led_g          = DEFAULT_LED_G;
    g_config.led_b          = DEFAULT_LED_B;
    g_config.led_toggle     = DEFAULT_LED_TOGGLE;
    g_config.key1_long_mod    = DEFAULT_KEY1_LONG_MOD;
    g_config.key1_long_code   = DEFAULT_KEY1_LONG;
    g_config.key2_long_mod    = DEFAULT_KEY2_LONG_MOD;
    g_config.key2_long_code   = DEFAULT_KEY2_LONG;
    g_config.key3_long_mod    = DEFAULT_KEY3_LONG_MOD;
    g_config.key3_long_code   = DEFAULT_KEY3_LONG;
    g_config.enc_btn_long_mod = DEFAULT_ENC_BTN_LONG_MOD;
    g_config.enc_btn_long_code= DEFAULT_ENC_BTN_LONG;
    config_save();
}

/* ---- Feature Report 2 pack/unpack (key mapping) ---- */
void config_pack_report2(uint8_t *buf) {
    buf[0] = g_config.key1_mod;
    buf[1] = g_config.key1_code;
    buf[2] = g_config.key2_mod;
    buf[3] = g_config.key2_code;
    buf[4] = g_config.key3_mod;
    buf[5] = g_config.key3_code;
    buf[6] = g_config.enc_btn_mod;
}

void config_unpack_report2(const uint8_t *buf) {
    g_config.key1_mod     = buf[0];
    g_config.key1_code    = buf[1];
    g_config.key2_mod     = buf[2];
    g_config.key2_code    = buf[3];
    g_config.key3_mod     = buf[4];
    g_config.key3_code    = buf[5];
    g_config.enc_btn_mod  = buf[6];
}

/* ---- Feature Report 3 pack/unpack (encoder mapping) ---- */
void config_pack_report3(uint8_t *buf) {
    buf[0] = g_config.enc_btn_code;
    buf[1] = g_config.enc_cw_mod;
    buf[2] = g_config.enc_cw_key;
    buf[3] = g_config.enc_ccw_mod;
    buf[4] = g_config.enc_ccw_key;
    buf[5] = 0;
    buf[6] = 0;
}

void config_unpack_report3(const uint8_t *buf) {
    g_config.enc_btn_code   = buf[0];
    g_config.enc_cw_mod     = buf[1];
    g_config.enc_cw_key     = buf[2];
    g_config.enc_ccw_mod    = buf[3];
    g_config.enc_ccw_key    = buf[4];
}

/* ---- Feature Report 5 pack/unpack (LED config) ---- */
void config_pack_report5(uint8_t *buf) {
    buf[0] = g_config.led_brightness;
    buf[1] = g_config.led_mode;
    buf[2] = g_config.led_r;
    buf[3] = g_config.led_g;
    buf[4] = g_config.led_b;
    buf[5] = g_config.led_toggle;
    buf[6] = 0;
}

void config_unpack_report5(const uint8_t *buf) {
    g_config.led_brightness = buf[0];
    g_config.led_mode       = buf[1];
    g_config.led_r          = buf[2];
    g_config.led_g          = buf[3];
    g_config.led_b          = buf[4];
    g_config.led_toggle     = buf[5];
    /* Validate led_mode */
    if (g_config.led_mode > LED_MODE_BREATHE)
        g_config.led_mode = LED_MODE_RAINBOW;
}

/* ---- Feature Report 6 pack/unpack (long-press key mapping) ---- */
void config_pack_report6(uint8_t *buf) {
    buf[0] = g_config.key1_long_mod;
    buf[1] = g_config.key1_long_code;
    buf[2] = g_config.key2_long_mod;
    buf[3] = g_config.key2_long_code;
    buf[4] = g_config.key3_long_mod;
    buf[5] = g_config.key3_long_code;
    buf[6] = g_config.enc_btn_long_mod;
}

void config_unpack_report6(const uint8_t *buf) {
    g_config.key1_long_mod    = buf[0];
    g_config.key1_long_code   = buf[1];
    g_config.key2_long_mod    = buf[2];
    g_config.key2_long_code   = buf[3];
    g_config.key3_long_mod    = buf[4];
    g_config.key3_long_code   = buf[5];
    g_config.enc_btn_long_mod = buf[6];
}

/* ---- Feature Report 7 pack/unpack (long-press encoder) ---- */
void config_pack_report7(uint8_t *buf) {
    buf[0] = g_config.enc_btn_long_code;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 0;
    buf[4] = 0;
    buf[5] = 0;
    buf[6] = 0;
}

void config_unpack_report7(const uint8_t *buf) {
    g_config.enc_btn_long_code = buf[0];
}
