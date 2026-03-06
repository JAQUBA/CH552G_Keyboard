/*
 * keymap.cpp — VK ↔ firmware key code conversions
 */
#include "keymap.h"
#include "protocol.h"

uint8_t vkToFwKey(UINT vk) {
    if (vk >= 'A' && vk <= 'Z') return (uint8_t)('a' + (vk - 'A'));
    if (vk >= '0' && vk <= '9') return (uint8_t)vk;
    switch (vk) {
        case VK_SPACE:   return ' ';
        case VK_RETURN:  return KC_RETURN;
        case VK_ESCAPE:  return KC_ESC;
        case VK_BACK:    return KC_BACKSPACE;
        case VK_TAB:     return KC_TAB;
        case VK_UP:      return KC_UP_ARROW;
        case VK_DOWN:    return KC_DOWN_ARROW;
        case VK_LEFT:    return KC_LEFT_ARROW;
        case VK_RIGHT:   return KC_RIGHT_ARROW;
        case VK_INSERT:  return KC_INSERT;
        case VK_DELETE:  return KC_DELETE;
        case VK_HOME:    return KC_HOME;
        case VK_END:     return KC_END;
        case VK_PRIOR:   return KC_PAGE_UP;
        case VK_NEXT:    return KC_PAGE_DOWN;
        case VK_CAPITAL: return KC_CAPS_LOCK;
        case VK_F1:  return KC_F1;  case VK_F2:  return KC_F2;
        case VK_F3:  return KC_F3;  case VK_F4:  return KC_F4;
        case VK_F5:  return KC_F5;  case VK_F6:  return KC_F6;
        case VK_F7:  return KC_F7;  case VK_F8:  return KC_F8;
        case VK_F9:  return KC_F9;  case VK_F10: return KC_F10;
        case VK_F11: return KC_F11; case VK_F12: return KC_F12;
        case VK_LCONTROL: return KC_LEFT_CTRL;
        case VK_RCONTROL: return KC_RIGHT_CTRL;
        case VK_LSHIFT:   return KC_LEFT_SHIFT;
        case VK_RSHIFT:   return KC_RIGHT_SHIFT;
        case VK_LMENU:    return KC_LEFT_ALT;
        case VK_RMENU:    return KC_RIGHT_ALT;
        case VK_LWIN:     return KC_LEFT_GUI;
        case VK_RWIN:     return KC_RIGHT_GUI;
        case VK_OEM_MINUS:  return '-';
        case VK_OEM_PLUS:   return '=';
        case VK_OEM_4:      return '[';
        case VK_OEM_6:      return ']';
        case VK_OEM_1:      return ';';
        case VK_OEM_7:      return '\'';
        case VK_OEM_COMMA:  return ',';
        case VK_OEM_PERIOD: return '.';
        case VK_OEM_2:      return '/';
        case VK_OEM_5:      return '\\';
        case VK_OEM_3:      return '`';
        case VK_VOLUME_MUTE:       return KC_MEDIA_MUTE;
        case VK_VOLUME_DOWN:       return KC_MEDIA_VOL_DOWN;
        case VK_VOLUME_UP:         return KC_MEDIA_VOL_UP;
        case VK_MEDIA_NEXT_TRACK:  return KC_MEDIA_NEXT_TRACK;
        case VK_MEDIA_PREV_TRACK:  return KC_MEDIA_PREV_TRACK;
        case VK_MEDIA_STOP:        return KC_MEDIA_STOP;
        case VK_MEDIA_PLAY_PAUSE:  return KC_MEDIA_PLAY_PAUSE;
        default: return 0;
    }
}

UINT fwKeyToVk(uint8_t code) {
    if (code >= 'a' && code <= 'z') return (UINT)('A' + (code - 'a'));
    if (code >= '0' && code <= '9') return (UINT)code;
    if (code == ' ')  return VK_SPACE;
    if (code == '-')  return VK_OEM_MINUS;
    if (code == '=')  return VK_OEM_PLUS;
    if (code == '[')  return VK_OEM_4;
    if (code == ']')  return VK_OEM_6;
    if (code == ';')  return VK_OEM_1;
    if (code == '\'') return VK_OEM_7;
    if (code == ',')  return VK_OEM_COMMA;
    if (code == '.')  return VK_OEM_PERIOD;
    if (code == '/')  return VK_OEM_2;
    if (code == '\\') return VK_OEM_5;
    if (code == '`')  return VK_OEM_3;
    switch (code) {
        case KC_RETURN:      return VK_RETURN;
        case KC_ESC:         return VK_ESCAPE;
        case KC_BACKSPACE:   return VK_BACK;
        case KC_TAB:         return VK_TAB;
        case KC_UP_ARROW:    return VK_UP;
        case KC_DOWN_ARROW:  return VK_DOWN;
        case KC_LEFT_ARROW:  return VK_LEFT;
        case KC_RIGHT_ARROW: return VK_RIGHT;
        case KC_INSERT:      return VK_INSERT;
        case KC_DELETE:      return VK_DELETE;
        case KC_HOME:        return VK_HOME;
        case KC_END:         return VK_END;
        case KC_PAGE_UP:     return VK_PRIOR;
        case KC_PAGE_DOWN:   return VK_NEXT;
        case KC_CAPS_LOCK:   return VK_CAPITAL;
        case KC_LEFT_CTRL:   return VK_LCONTROL;
        case KC_LEFT_SHIFT:  return VK_LSHIFT;
        case KC_LEFT_ALT:    return VK_LMENU;
        case KC_LEFT_GUI:    return VK_LWIN;
        case KC_RIGHT_CTRL:  return VK_RCONTROL;
        case KC_RIGHT_SHIFT: return VK_RSHIFT;
        case KC_RIGHT_ALT:   return VK_RMENU;
        case KC_RIGHT_GUI:   return VK_RWIN;
        case KC_MEDIA_PLAY_PAUSE: return VK_MEDIA_PLAY_PAUSE;
        case KC_MEDIA_STOP:       return VK_MEDIA_STOP;
        case KC_MEDIA_PREV_TRACK: return VK_MEDIA_PREV_TRACK;
        case KC_MEDIA_NEXT_TRACK: return VK_MEDIA_NEXT_TRACK;
        case KC_MEDIA_VOL_UP:     return VK_VOLUME_UP;
        case KC_MEDIA_VOL_DOWN:   return VK_VOLUME_DOWN;
        case KC_MEDIA_MUTE:       return VK_VOLUME_MUTE;
        case KC_F1:  return VK_F1;  case KC_F2:  return VK_F2;
        case KC_F3:  return VK_F3;  case KC_F4:  return VK_F4;
        case KC_F5:  return VK_F5;  case KC_F6:  return VK_F6;
        case KC_F7:  return VK_F7;  case KC_F8:  return VK_F8;
        case KC_F9:  return VK_F9;  case KC_F10: return VK_F10;
        case KC_F11: return VK_F11; case KC_F12: return VK_F12;
        default: return 0;
    }
}
