/*
 * keymap.h — VK ↔ firmware key code conversions
 */
#ifndef KEYMAP_H
#define KEYMAP_H

#include <windows.h>
#include <stdint.h>

/* Windows VK → firmware key code (0 if unmapped) */
uint8_t vkToFwKey(UINT vk);

/* Firmware key code → Windows VK (0 if unmapped) */
UINT fwKeyToVk(uint8_t code);

#endif /* KEYMAP_H */
