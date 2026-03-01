/*
 * config.h — EEPROM-backed keyboard configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* Runtime config (kept in XRAM) */
typedef struct {
    uint8_t key1_code;
    uint8_t key2_code;
    uint8_t key3_code;
    uint8_t enc_btn_code;
    uint8_t enc_cw_mod;
    uint8_t enc_cw_key;
    uint8_t enc_ccw_mod;
    uint8_t enc_ccw_key;
    uint8_t led_brightness;
    uint8_t led_mode;
    uint8_t led_r;
    uint8_t led_g;
    uint8_t led_b;
    uint8_t led_toggle;  /* bitmask: bit0=key1, bit1=key2, bit2=key3 */
} KeyboardConfig;

extern __xdata KeyboardConfig g_config;

/* Load config from DataFlash; loads defaults if magic is invalid. */
void config_load(void);

/* Save current g_config to DataFlash. */
void config_save(void);

/* Reset g_config to factory defaults and save. */
void config_reset(void);

/* Pack g_config into Feature Report 2 buffer (7 bytes). */
void config_pack_report2(uint8_t *buf);

/* Pack g_config into Feature Report 3 buffer (7 bytes). */
void config_pack_report3(uint8_t *buf);

/* Unpack Feature Report 2 data (7 bytes) into g_config. */
void config_unpack_report2(const uint8_t *buf);

/* Unpack Feature Report 3 data (7 bytes) into g_config. */
void config_unpack_report3(const uint8_t *buf);

#endif /* CONFIG_H */
