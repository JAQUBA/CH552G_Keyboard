/*
 * main.cpp — CH552G Keyboard Configurator  (Windows, JQB_WindowsLib)
 *
 * Connects to the keyboard via HID Feature Reports (Interface 1)
 * and allows configuring key assignments, encoder and LED settings.
 * Uses tabbed UI with keyboard shortcut capture for key assignment.
 *
 * All key bindings support modifier combinations (e.g. Ctrl+Alt+F6).
 */

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/TabControl/TabControl.h>
#include <UI/Button/Button.h>
#include <UI/Label/Label.h>
#include <UI/Select/Select.h>
#include <IO/HID/HID.h>
#include <Util/StringUtils.h>

#include "protocol.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <map>
#include <commctrl.h>

/* ================================================================== */
/*  Key code ↔ display name conversion                                */
/* ================================================================== */

/* VK → firmware key code mapping */
static uint8_t vkToFwKey(UINT vk) {
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
        case VK_F1:      return KC_F1;
        case VK_F2:      return KC_F2;
        case VK_F3:      return KC_F3;
        case VK_F4:      return KC_F4;
        case VK_F5:      return KC_F5;
        case VK_F6:      return KC_F6;
        case VK_F7:      return KC_F7;
        case VK_F8:      return KC_F8;
        case VK_F9:      return KC_F9;
        case VK_F10:     return KC_F10;
        case VK_F11:     return KC_F11;
        case VK_F12:     return KC_F12;
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
        default: return 0;
    }
}

/* Firmware key code → display name (single key, no modifier prefix) */
static std::string fwKeyToName(uint8_t code) {
    if (code == 0) return "(brak)";
    if (code >= 'a' && code <= 'z') {
        char s[2] = {(char)(code - 32), 0};
        return s;
    }
    if (code >= '0' && code <= '9') {
        char s[2] = {(char)code, 0};
        return s;
    }
    if (code == ' ') return "Spacja";
    if (code == '-' || code == '=' || code == '[' || code == ']' ||
        code == ';' || code == '\'' || code == ',' || code == '.' ||
        code == '/' || code == '\\' || code == '`') {
        char s[2] = {(char)code, 0};
        return s;
    }
    switch (code) {
        case KC_RETURN:      return "Enter";
        case KC_ESC:         return "Escape";
        case KC_BACKSPACE:   return "Backspace";
        case KC_TAB:         return "Tab";
        case KC_UP_ARROW:    return "Up";
        case KC_DOWN_ARROW:  return "Down";
        case KC_LEFT_ARROW:  return "Left";
        case KC_RIGHT_ARROW: return "Right";
        case KC_INSERT:      return "Insert";
        case KC_DELETE:      return "Delete";
        case KC_HOME:        return "Home";
        case KC_END:         return "End";
        case KC_PAGE_UP:     return "PageUp";
        case KC_PAGE_DOWN:   return "PageDown";
        case KC_CAPS_LOCK:   return "CapsLock";
        case KC_LEFT_CTRL:   return "LCtrl";
        case KC_LEFT_SHIFT:  return "LShift";
        case KC_LEFT_ALT:    return "LAlt";
        case KC_LEFT_GUI:    return "LWin";
        case KC_RIGHT_CTRL:  return "RCtrl";
        case KC_RIGHT_SHIFT: return "RShift";
        case KC_RIGHT_ALT:   return "RAlt";
        case KC_RIGHT_GUI:   return "RWin";
        case KC_F1:  return "F1";   case KC_F2:  return "F2";
        case KC_F3:  return "F3";   case KC_F4:  return "F4";
        case KC_F5:  return "F5";   case KC_F6:  return "F6";
        case KC_F7:  return "F7";   case KC_F8:  return "F8";
        case KC_F9:  return "F9";   case KC_F10: return "F10";
        case KC_F11: return "F11";  case KC_F12: return "F12";
        default: {
            char buf[8];
            snprintf(buf, sizeof(buf), "0x%02X", code);
            return buf;
        }
    }
}

/* ================================================================== */
/*  Modifier bitmask helpers                                           */
/* ================================================================== */

/* Map firmware key code for a modifier to its MOD_BIT_* value.
   Returns 0 if the key is not a modifier. */
static uint8_t fwKeyToModBit(uint8_t code) {
    if (code >= KC_LEFT_CTRL && code <= KC_RIGHT_GUI)
        return (uint8_t)(1 << (code - KC_LEFT_CTRL));
    return 0;
}

/* Build a display string for modifier bitmask, e.g. "Ctrl+Alt+" */
static std::string modBitsToPrefix(uint8_t bits) {
    std::string s;
    if (bits & MOD_BIT_LCTRL)  s += "LCtrl+";
    if (bits & MOD_BIT_RCTRL)  s += "RCtrl+";
    if (bits & MOD_BIT_LSHIFT) s += "LShift+";
    if (bits & MOD_BIT_RSHIFT) s += "RShift+";
    if (bits & MOD_BIT_LALT)   s += "LAlt+";
    if (bits & MOD_BIT_RALT)   s += "RAlt+";
    if (bits & MOD_BIT_LGUI)   s += "LWin+";
    if (bits & MOD_BIT_RGUI)   s += "RWin+";
    return s;
}

/* Full display name for a key combo: "LCtrl+LAlt+F6" or just "B" */
static std::string comboToName(uint8_t mod, uint8_t code) {
    std::string prefix = modBitsToPrefix(mod);
    std::string keyName = fwKeyToName(code);
    return prefix + keyName;
}

/* Read current modifier state from keyboard hardware */
static uint8_t getAsyncModBits() {
    uint8_t bits = 0;
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) bits |= MOD_BIT_LCTRL;
    if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) bits |= MOD_BIT_RCTRL;
    if (GetAsyncKeyState(VK_LSHIFT)   & 0x8000) bits |= MOD_BIT_LSHIFT;
    if (GetAsyncKeyState(VK_RSHIFT)   & 0x8000) bits |= MOD_BIT_RSHIFT;
    if (GetAsyncKeyState(VK_LMENU)    & 0x8000) bits |= MOD_BIT_LALT;
    if (GetAsyncKeyState(VK_RMENU)    & 0x8000) bits |= MOD_BIT_RALT;
    if (GetAsyncKeyState(VK_LWIN)     & 0x8000) bits |= MOD_BIT_LGUI;
    if (GetAsyncKeyState(VK_RWIN)     & 0x8000) bits |= MOD_BIT_RGUI;
    return bits;
}

/* ================================================================== */
/*  Key-capture button: click to start, press combo to assign          */
/* ================================================================== */

struct KeyCapture {
    HWND    hwndBtn;         /* the button showing the combo name */
    uint8_t mod;             /* modifier bitmask                  */
    uint8_t code;            /* firmware key code                 */
    bool    listening;       /* waiting for keypress              */
    uint8_t pendingMod;      /* modifier preview while listening  */
    uint8_t lastModKey;      /* last modifier key pressed alone   */
};

static LRESULT CALLBACK KeyCaptureProc(HWND hwnd, UINT msg, WPARAM wp,
                                        LPARAM lp, UINT_PTR idSubclass,
                                        DWORD_PTR refData);

static void keyCaptureSetCombo(KeyCapture* kc, uint8_t mod, uint8_t code) {
    kc->mod  = mod;
    kc->code = code;
    kc->listening = false;
    std::string label = comboToName(mod, code);
    std::wstring wLabel = StringUtils::utf8ToWide(label);
    SetWindowTextW(kc->hwndBtn, wLabel.c_str());
}

static KeyCapture* createKeyCapture(HWND parent, int x, int y, int w,
                                     int h, uint8_t initMod,
                                     uint8_t initCode) {
    KeyCapture* kc = new KeyCapture();
    kc->mod  = initMod;
    kc->code = initCode;
    kc->listening = false;
    kc->pendingMod = 0;
    kc->lastModKey = 0;

    std::string label = comboToName(initMod, initCode);
    std::wstring wLabel = StringUtils::utf8ToWide(label);

    kc->hwndBtn = CreateWindowExW(0, L"BUTTON", wLabel.c_str(),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        x, y, w, h, parent, NULL, _core.hInstance, NULL);

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (hFont) SendMessageW(kc->hwndBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

    SetWindowSubclass(kc->hwndBtn, KeyCaptureProc, 0, (DWORD_PTR)kc);
    return kc;
}

static LRESULT CALLBACK KeyCaptureProc(HWND hwnd, UINT msg, WPARAM wp,
                                        LPARAM lp, UINT_PTR idSubclass,
                                        DWORD_PTR refData) {
    KeyCapture* kc = (KeyCapture*)refData;

    switch (msg) {
        case WM_LBUTTONDOWN:
            /* Start listening */
            kc->listening = true;
            kc->pendingMod = 0;
            kc->lastModKey = 0;
            SetWindowTextW(hwnd, L"...");
            SetFocus(hwnd);
            return 0;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (kc->listening) {
                UINT vk = (UINT)wp;
                /* Distinguish left/right modifiers */
                if (vk == VK_SHIFT) {
                    vk = (MapVirtualKeyW((lp >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX)
                          == VK_RSHIFT) ? VK_RSHIFT : VK_LSHIFT;
                } else if (vk == VK_CONTROL) {
                    vk = (lp & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
                } else if (vk == VK_MENU) {
                    vk = (lp & (1 << 24)) ? VK_RMENU : VK_LMENU;
                }
                uint8_t fwCode = vkToFwKey(vk);
                if (fwCode == 0) return 0;

                uint8_t keyBit = fwKeyToModBit(fwCode);
                if (keyBit) {
                    /* Modifier pressed — show preview, keep listening */
                    kc->pendingMod = getAsyncModBits();
                    kc->lastModKey = fwCode;
                    std::string preview = modBitsToPrefix(kc->pendingMod) + "...";
                    std::wstring wPreview = StringUtils::utf8ToWide(preview);
                    SetWindowTextW(hwnd, wPreview.c_str());
                } else {
                    /* Non-modifier pressed — commit combo and stop */
                    uint8_t modBits = getAsyncModBits();
                    keyCaptureSetCombo(kc, modBits, fwCode);
                }
                return 0;
            }
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (kc->listening) {
                /* If all modifiers released, commit last modifier as
                   standalone key (e.g. user pressed only LCtrl). */
                uint8_t nowHeld = getAsyncModBits();
                if (nowHeld == 0 && kc->lastModKey != 0) {
                    uint8_t keyBit = fwKeyToModBit(kc->lastModKey);
                    uint8_t modBits = kc->pendingMod & ~keyBit;
                    keyCaptureSetCombo(kc, modBits, kc->lastModKey);
                } else {
                    /* Still holding some modifiers — update preview */
                    kc->pendingMod = nowHeld;
                    std::string preview = modBitsToPrefix(nowHeld) + "...";
                    std::wstring wPreview = StringUtils::utf8ToWide(preview);
                    SetWindowTextW(hwnd, wPreview.c_str());
                }
                return 0;
            }
            break;

        case WM_KILLFOCUS:
            if (kc->listening) {
                /* Cancel capture on focus loss */
                kc->listening = false;
                std::string label = comboToName(kc->mod, kc->code);
                std::wstring wLabel = StringUtils::utf8ToWide(label);
                SetWindowTextW(hwnd, wLabel.c_str());
            }
            break;

        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, KeyCaptureProc, idSubclass);
            delete kc;
            break;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

/* ================================================================== */
/*  Tables                                                             */
/* ================================================================== */

struct KeyDef { const char* name; uint8_t code; };

static const KeyDef ledModeTable[] = {
    {"Tecza",     LED_MODE_RAINBOW},
    {"Statyczny", LED_MODE_STATIC},
    {"Oddech",    LED_MODE_BREATHE},
};
static const int LED_MODE_COUNT = sizeof(ledModeTable) / sizeof(ledModeTable[0]);

struct ColorDef { const char* name; uint8_t r, g, b; };
static const ColorDef colorTable[] = {
    {"Bialy",        255, 255, 255},
    {"Czerwony",     255,   0,   0},
    {"Zielony",        0, 255,   0},
    {"Niebieski",      0,   0, 255},
    {"Zolty",        255, 255,   0},
    {"Cyan",           0, 255, 255},
    {"Magenta",      255,   0, 255},
    {"Pomaranczowy", 255, 128,   0},
    {"Fioletowy",    128,   0, 255},
};
static const int COLOR_COUNT = sizeof(colorTable) / sizeof(colorTable[0]);

/* ================================================================== */
/*  Helpers                                                            */
/* ================================================================== */

static int findLedModeIndex(uint8_t code) {
    for (int i = 0; i < LED_MODE_COUNT; i++)
        if (ledModeTable[i].code == code) return i;
    return 0;
}
static int findColorIndex(uint8_t r, uint8_t g, uint8_t b) {
    int best = 0, bestDist = 999999;
    for (int i = 0; i < COLOR_COUNT; i++) {
        int dr = (int)colorTable[i].r - r;
        int dg = (int)colorTable[i].g - g;
        int db = (int)colorTable[i].b - b;
        int d = dr*dr + dg*dg + db*db;
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}

static int getSelIndex(Select* sel) {
    return (int)SendMessageW(sel->getHandle(), CB_GETCURSEL, 0, 0);
}
static void setSelIndex(Select* sel, int idx) {
    SendMessageW(sel->getHandle(), CB_SETCURSEL, (WPARAM)idx, 0);
}

/* Create a static label on a raw HWND parent (tab page) */
static HWND createLabel(HWND parent, int x, int y, int w, int h,
                        const wchar_t* text) {
    HWND hw = CreateWindowExW(0, L"STATIC", text,
        WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent, NULL, _core.hInstance, NULL);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (hFont) SendMessageW(hw, WM_SETFONT, (WPARAM)hFont, TRUE);
    return hw;
}

/* Create a raw checkbox on a tab page */
static HWND createCheckBox(HWND parent, int x, int y, int w, int h,
                           const wchar_t* text, bool checked) {
    HWND hw = CreateWindowExW(0, L"BUTTON", text,
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        x, y, w, h, parent, NULL, _core.hInstance, NULL);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (hFont) SendMessageW(hw, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (checked)
        SendMessageW(hw, BM_SETCHECK, BST_CHECKED, 0);
    return hw;
}

static bool isChecked(HWND hw) {
    return SendMessageW(hw, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

/* ================================================================== */
/*  Globals                                                            */
/* ================================================================== */

static HID           hid;
static SimpleWindow* window;
static TabControl*   tabs;

/* Status bar */
static Label*  lblStatus;
static Button* btnConnect;

/* Tab 0 — Klawisze */
static KeyCapture* kcKey1;
static KeyCapture* kcKey2;
static KeyCapture* kcKey3;
static KeyCapture* kcEncBtn;
static KeyCapture* kcEncCw;
static KeyCapture* kcEncCcw;

/* Tab 1 — LED */
static Select* selLedMode;
static Select* selBrightness;
static Select* selLedColor;

/* Tab 2 — LED Toggle */
static HWND chkToggle1;
static HWND chkToggle2;
static HWND chkToggle3;

/* Bottom buttons */
static Button* btnRead;
static Button* btnWrite;
static Button* btnReset;
static Button* btnBootloader;

/* Polling state for auto-send on LED change */
static int prevLedModeIdx  = -1;
static int prevBrtIdx      = -1;
static int prevColorIdx    = -1;
static bool prevTog1       = false;
static bool prevTog2       = false;
static bool prevTog3       = false;
static bool ignoreSelChange = false; /* suppress during readFromDevice */

/* ================================================================== */
/*  Auto-connect helper                                                */
/* ================================================================== */

static bool tryConnect() {
    if (hid.isOpen()) return true;
    if (hid.findAndOpen()) {
        lblStatus->setText(L"Status: Polaczony");
        SetWindowTextA(btnConnect->getHandle(), "Rozlacz");
        return true;
    }
    lblStatus->setText(L"Nie znaleziono klawiatury!");
    return false;
}

/* ================================================================== */
/*  Send only LED config (Report 5) — used for live preview            */
/* ================================================================== */

static void sendLedConfig() {
    if (!hid.isOpen()) return;

    int idx;
    uint8_t buf5[CONFIG_REPORT_SIZE];
    memset(buf5, 0, sizeof(buf5));

    idx = getSelIndex(selBrightness);
    if (idx >= 0 && idx <= 25) {
        buf5[0] = (uint8_t)(idx * 10);
        if (idx == 25) buf5[0] = 255;
    } else {
        buf5[0] = DEFAULT_LED_BRT;
    }

    idx = getSelIndex(selLedMode);
    buf5[1] = (idx >= 0 && idx < LED_MODE_COUNT)
              ? ledModeTable[idx].code : DEFAULT_LED_MODE;

    idx = getSelIndex(selLedColor);
    if (idx >= 0 && idx < COLOR_COUNT) {
        buf5[2] = colorTable[idx].r;
        buf5[3] = colorTable[idx].g;
        buf5[4] = colorTable[idx].b;
    } else {
        buf5[2] = DEFAULT_LED_R;
        buf5[3] = DEFAULT_LED_G;
        buf5[4] = DEFAULT_LED_B;
    }

    buf5[5] = 0;
    if (isChecked(chkToggle1)) buf5[5] |= 0x01;
    if (isChecked(chkToggle2)) buf5[5] |= 0x02;
    if (isChecked(chkToggle3)) buf5[5] |= 0x04;

    hid.setFeatureReport(REPORT_ID_LED, buf5, CONFIG_REPORT_SIZE);
}

/* ================================================================== */
/*  Read / write helpers                                               */
/* ================================================================== */

static void readFromDevice() {
    if (!tryConnect()) return;

    uint8_t buf2[CONFIG_REPORT_SIZE];
    uint8_t buf3[CONFIG_REPORT_SIZE];
    uint8_t buf5[CONFIG_REPORT_SIZE];

    if (!hid.getFeatureReport(REPORT_ID_KEYS, buf2, CONFIG_REPORT_SIZE) ||
        !hid.getFeatureReport(REPORT_ID_ENCODER, buf3, CONFIG_REPORT_SIZE) ||
        !hid.getFeatureReport(REPORT_ID_LED, buf5, CONFIG_REPORT_SIZE)) {
        lblStatus->setText(L"Blad odczytu!");
        return;
    }

    /* Report 2: key mapping (mod+code pairs) */
    keyCaptureSetCombo(kcKey1,   buf2[0], buf2[1]);
    keyCaptureSetCombo(kcKey2,   buf2[2], buf2[3]);
    keyCaptureSetCombo(kcKey3,   buf2[4], buf2[5]);

    /* Report 3: encoder mapping */
    keyCaptureSetCombo(kcEncBtn, buf2[6], buf3[0]);  /* mod in report2[6], code in report3[0] */
    keyCaptureSetCombo(kcEncCw,  buf3[1], buf3[2]);
    keyCaptureSetCombo(kcEncCcw, buf3[3], buf3[4]);

    /* Report 5: LED config */
    {
        int brtIdx = buf5[0] / 10;
        if (brtIdx > 25) brtIdx = 25;
        setSelIndex(selBrightness, brtIdx);
    }

    setSelIndex(selLedMode,  findLedModeIndex(buf5[1]));
    setSelIndex(selLedColor, findColorIndex(buf5[2], buf5[3], buf5[4]));

    /* LED toggle checkboxes */
    SendMessageW(chkToggle1, BM_SETCHECK,
                 (buf5[5] & 0x01) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(chkToggle2, BM_SETCHECK,
                 (buf5[5] & 0x02) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(chkToggle3, BM_SETCHECK,
                 (buf5[5] & 0x04) ? BST_CHECKED : BST_UNCHECKED, 0);

    /* Sync polling state so loop() doesn't immediately re-send */
    prevLedModeIdx = getSelIndex(selLedMode);
    prevBrtIdx     = getSelIndex(selBrightness);
    prevColorIdx   = getSelIndex(selLedColor);
    prevTog1       = isChecked(chkToggle1);
    prevTog2       = isChecked(chkToggle2);
    prevTog3       = isChecked(chkToggle3);

    lblStatus->setText(L"Odczytano z urzadzenia");
}

static void writeToDevice() {
    if (!tryConnect()) return;

    int idx;
    uint8_t buf2[CONFIG_REPORT_SIZE];
    uint8_t buf3[CONFIG_REPORT_SIZE];
    uint8_t buf5[CONFIG_REPORT_SIZE];
    memset(buf2, 0, sizeof(buf2));
    memset(buf3, 0, sizeof(buf3));
    memset(buf5, 0, sizeof(buf5));

    /* Report 2: key mapping */
    buf2[0] = kcKey1->mod;
    buf2[1] = kcKey1->code;
    buf2[2] = kcKey2->mod;
    buf2[3] = kcKey2->code;
    buf2[4] = kcKey3->mod;
    buf2[5] = kcKey3->code;
    buf2[6] = kcEncBtn->mod;

    /* Report 3: encoder mapping */
    buf3[0] = kcEncBtn->code;
    buf3[1] = kcEncCw->mod;
    buf3[2] = kcEncCw->code;
    buf3[3] = kcEncCcw->mod;
    buf3[4] = kcEncCcw->code;

    /* Report 5: LED config */
    idx = getSelIndex(selBrightness);
    if (idx >= 0 && idx <= 25) {
        buf5[0] = (uint8_t)(idx * 10);
        if (idx == 25) buf5[0] = 255;
    } else {
        buf5[0] = DEFAULT_LED_BRT;
    }

    idx = getSelIndex(selLedMode);
    buf5[1] = (idx >= 0 && idx < LED_MODE_COUNT)
              ? ledModeTable[idx].code : DEFAULT_LED_MODE;

    idx = getSelIndex(selLedColor);
    if (idx >= 0 && idx < COLOR_COUNT) {
        buf5[2] = colorTable[idx].r;
        buf5[3] = colorTable[idx].g;
        buf5[4] = colorTable[idx].b;
    } else {
        buf5[2] = DEFAULT_LED_R;
        buf5[3] = DEFAULT_LED_G;
        buf5[4] = DEFAULT_LED_B;
    }

    /* LED toggle bitmask */
    buf5[5] = 0;
    if (isChecked(chkToggle1)) buf5[5] |= 0x01;
    if (isChecked(chkToggle2)) buf5[5] |= 0x02;
    if (isChecked(chkToggle3)) buf5[5] |= 0x04;

    if (!hid.setFeatureReport(REPORT_ID_KEYS, buf2, CONFIG_REPORT_SIZE) ||
        !hid.setFeatureReport(REPORT_ID_ENCODER, buf3, CONFIG_REPORT_SIZE) ||
        !hid.setFeatureReport(REPORT_ID_LED, buf5, CONFIG_REPORT_SIZE)) {
        lblStatus->setText(L"Blad zapisu!");
        return;
    }
    lblStatus->setText(L"Zapisano do urzadzenia (EEPROM)");

    /* Sync polling state */
    prevLedModeIdx = getSelIndex(selLedMode);
    prevBrtIdx     = getSelIndex(selBrightness);
    prevColorIdx   = getSelIndex(selLedColor);
    prevTog1       = isChecked(chkToggle1);
    prevTog2       = isChecked(chkToggle2);
    prevTog3       = isChecked(chkToggle3);
}

static void resetDefaults() {
    keyCaptureSetCombo(kcKey1,   DEFAULT_KEY1_MOD,    DEFAULT_KEY1);
    keyCaptureSetCombo(kcKey2,   DEFAULT_KEY2_MOD,    DEFAULT_KEY2);
    keyCaptureSetCombo(kcKey3,   DEFAULT_KEY3_MOD,    DEFAULT_KEY3);
    keyCaptureSetCombo(kcEncBtn, DEFAULT_ENC_BTN_MOD, DEFAULT_ENC_BTN);
    keyCaptureSetCombo(kcEncCw,  DEFAULT_CW_MOD,      DEFAULT_CW_KEY);
    keyCaptureSetCombo(kcEncCcw, DEFAULT_CCW_MOD,     DEFAULT_CCW_KEY);
    setSelIndex(selBrightness, DEFAULT_LED_BRT / 10);
    setSelIndex(selLedMode,    findLedModeIndex(DEFAULT_LED_MODE));
    setSelIndex(selLedColor,
                findColorIndex(DEFAULT_LED_R, DEFAULT_LED_G, DEFAULT_LED_B));

    SendMessageW(chkToggle1, BM_SETCHECK,
        (DEFAULT_LED_TOGGLE & 0x01) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(chkToggle2, BM_SETCHECK,
        (DEFAULT_LED_TOGGLE & 0x02) ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(chkToggle3, BM_SETCHECK,
        (DEFAULT_LED_TOGGLE & 0x04) ? BST_CHECKED : BST_UNCHECKED, 0);

    lblStatus->setText(L"Przywrocono domyslne (kliknij Zapisz)");
}

/* ================================================================== */
/*  JQB_WindowsLib entry points                                        */
/* ================================================================== */

void init() {}

void setup() {
    hid.init();
    hid.setVidPid(KB_VID, KB_PID);
    hid.setUsage(VENDOR_USAGE_PAGE, VENDOR_USAGE);
    hid.setFeatureReportSize(CONFIG_REPORT_SIZE);

    window = new SimpleWindow(510, 530, "CH552G Keyboard Configurator", 0);
    window->init();

    const int LX = 10;
    const int LW = 165;
    const int SX = 180;
    const int SW = 200;
    const int ROW = 30;
    const int SELDH = 300;

    /* ================================================================ */
    /*  Connection bar (top of window — always visible)                  */
    /* ================================================================ */

    lblStatus = new Label(LX, 8, 290, 22, L"Status: Rozlaczony");
    window->add(lblStatus);

    btnConnect = new Button(390, 6, 100, 25, "Polacz", [](Button*) {
        if (hid.isOpen()) {
            hid.close();
            lblStatus->setText(L"Status: Rozlaczony");
            SetWindowTextA(btnConnect->getHandle(), "Polacz");
        } else {
            if (hid.findAndOpen()) {
                lblStatus->setText(L"Status: Polaczony");
                SetWindowTextA(btnConnect->getHandle(), "Rozlacz");
            } else {
                lblStatus->setText(L"Nie znaleziono klawiatury!");
            }
        }
    });
    window->add(btnConnect);

    /* ================================================================ */
    /*  Tab control                                                      */
    /* ================================================================ */

    tabs = new TabControl(5, 36, 495, 400);
    window->add(tabs);
    tabs->addTab("Klawisze");    /* 0 */
    tabs->addTab("LED");         /* 1 */
    tabs->addTab("LED Toggle");  /* 2 */

    HWND tabKeys = tabs->getTabPage(0);
    HWND tabLed  = tabs->getTabPage(1);
    HWND tabTog  = tabs->getTabPage(2);

    /* ================================================================ */
    /*  Tab 0: Klawisze + Enkoder                                       */
    /* ================================================================ */
    int y = 10;

    createLabel(tabKeys, LX, y, LW, 20, L"Klawisz 1:");
    kcKey1 = createKeyCapture(tabKeys, SX, y - 2, SW, 24,
                              DEFAULT_KEY1_MOD, DEFAULT_KEY1);
    y += ROW;

    createLabel(tabKeys, LX, y, LW, 20, L"Klawisz 2:");
    kcKey2 = createKeyCapture(tabKeys, SX, y - 2, SW, 24,
                              DEFAULT_KEY2_MOD, DEFAULT_KEY2);
    y += ROW;

    createLabel(tabKeys, LX, y, LW, 20, L"Klawisz 3:");
    kcKey3 = createKeyCapture(tabKeys, SX, y - 2, SW, 24,
                              DEFAULT_KEY3_MOD, DEFAULT_KEY3);
    y += ROW;

    createLabel(tabKeys, LX, y, LW, 20, L"Przycisk enkodera:");
    kcEncBtn = createKeyCapture(tabKeys, SX, y - 2, SW, 24,
                                DEFAULT_ENC_BTN_MOD, DEFAULT_ENC_BTN);
    y += ROW + 12;

    createLabel(tabKeys, LX, y, LW, 20, L"Enkoder (prawo):");
    kcEncCw = createKeyCapture(tabKeys, SX, y - 2, SW, 24,
                               DEFAULT_CW_MOD, DEFAULT_CW_KEY);
    y += ROW;

    createLabel(tabKeys, LX, y, LW, 20, L"Enkoder (lewo):");
    kcEncCcw = createKeyCapture(tabKeys, SX, y - 2, SW, 24,
                                DEFAULT_CCW_MOD, DEFAULT_CCW_KEY);
    y += ROW + 10;

    createLabel(tabKeys, LX, y, LW + SW, 40,
        L"Kliknij przycisk, potem nacisnij\n"
        L"kombinacje klawiszy (np. Ctrl+F6).");

    /* ================================================================ */
    /*  Tab 1: LED                                                      */
    /* ================================================================ */
    y = 10;

    createLabel(tabLed, LX, y, LW, 20, L"Tryb:");
    selLedMode = new Select(SX, y - 2, SW, SELDH, "", nullptr);
    selLedMode->create(tabLed);
    y += ROW;

    createLabel(tabLed, LX, y, LW, 20, L"Jasnosc:");
    selBrightness = new Select(SX, y - 2, SW, SELDH, "", nullptr);
    selBrightness->create(tabLed);
    y += ROW;

    createLabel(tabLed, LX, y, LW, 20, L"Kolor:");
    selLedColor = new Select(SX, y - 2, SW, SELDH, "", nullptr);
    selLedColor->create(tabLed);
    y += ROW + 10;

    createLabel(tabLed, LX, y, 380, 40,
        L"Kolor dotyczy trybu Statyczny i Oddech.\n"
        L"Tecza ignoruje kolor.");

    /* Fill LED selects */
    for (int i = 0; i < LED_MODE_COUNT; i++)
        selLedMode->addItem(ledModeTable[i].name);
    for (int i = 0; i < COLOR_COUNT; i++)
        selLedColor->addItem(colorTable[i].name);
    {
        char buf[8];
        for (int v = 0; v <= 250; v += 10) {
            snprintf(buf, sizeof(buf), "%d", v);
            selBrightness->addItem(buf);
        }
        selBrightness->addItem("255");
    }

    /* ================================================================ */
    /*  Tab 2: LED Toggle                                               */
    /* ================================================================ */
    y = 10;
    createLabel(tabTog, LX, y, 400, 40,
        L"Wlacz toggle, by przycisk wlaczal/wylaczal\n"
        L"diode pod nim przy kazdym nacisnieciu.");
    y += 50;

    chkToggle1 = createCheckBox(tabTog, LX, y, 350, 24,
                                L"Klawisz 1 — toggle LED 1", false);
    y += ROW;
    chkToggle2 = createCheckBox(tabTog, LX, y, 350, 24,
                                L"Klawisz 2 — toggle LED 2", false);
    y += ROW;
    chkToggle3 = createCheckBox(tabTog, LX, y, 350, 24,
                                L"Klawisz 3 — toggle LED 3", false);

    /* ================================================================ */
    /*  Bottom action buttons (on main window, below tabs)               */
    /* ================================================================ */
    int by = 446;

    btnRead = new Button(LX, by, 110, 30, "Odczytaj",
        [](Button*) { readFromDevice(); });
    window->add(btnRead);

    btnWrite = new Button(130, by, 110, 30, "Zapisz",
        [](Button*) { writeToDevice(); });
    window->add(btnWrite);

    btnReset = new Button(250, by, 110, 30, "Domyslne",
        [](Button*) { resetDefaults(); });
    window->add(btnReset);

    btnBootloader = new Button(370, by, 120, 30, "Bootloader",
        [](Button*) {
            if (!tryConnect()) return;
            uint8_t cmdBuf[CMD_REPORT_SIZE];
            cmdBuf[0] = CMD_BOOTLOADER;
            /* Send bootloader command.  Firmware sets a flag and completes
               the USB transfer, then enters bootloader in its main loop.
               The device will disconnect shortly after we get the ACK. */
            bool ok = hid.setFeatureReport(REPORT_ID_COMMAND, cmdBuf,
                                           CMD_REPORT_SIZE);
            /* Close HID handle immediately — device is about to disappear */
            hid.close();
            SetWindowTextA(btnConnect->getHandle(), "Polacz");
            if (ok) {
                lblStatus->setText(L"Bootloader aktywny — mozna wgrac firmware");
            } else {
                /* Even if the call failed (device disconnected mid-transfer),
                   the command was likely received — device may already be
                   in bootloader mode. */
                lblStatus->setText(L"Wyslano komende bootloadera (urzadzenie odlaczone)");
            }
        });
    window->add(btnBootloader);

    /* Set defaults in UI */
    resetDefaults();

    /* Init polling state */
    prevLedModeIdx = getSelIndex(selLedMode);
    prevBrtIdx     = getSelIndex(selBrightness);
    prevColorIdx   = getSelIndex(selLedColor);
    prevTog1       = false;
    prevTog2       = false;
    prevTog3       = false;
}

void loop() {
    /* Auto-send LED config when any LED-related control changes */
    if (hid.isOpen() && selLedMode && selBrightness && selLedColor) {
        int curMode  = getSelIndex(selLedMode);
        int curBrt   = getSelIndex(selBrightness);
        int curColor = getSelIndex(selLedColor);
        bool curTog1 = isChecked(chkToggle1);
        bool curTog2 = isChecked(chkToggle2);
        bool curTog3 = isChecked(chkToggle3);

        if (curMode != prevLedModeIdx || curBrt != prevBrtIdx ||
            curColor != prevColorIdx || curTog1 != prevTog1 ||
            curTog2 != prevTog2 || curTog3 != prevTog3) {
            prevLedModeIdx = curMode;
            prevBrtIdx     = curBrt;
            prevColorIdx   = curColor;
            prevTog1       = curTog1;
            prevTog2       = curTog2;
            prevTog3       = curTog3;
            sendLedConfig();
        }
    }
}
