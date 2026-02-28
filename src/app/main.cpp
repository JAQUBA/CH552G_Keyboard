/*
 * main.cpp — CH552G Keyboard Configurator  (Windows, JQB_WindowsLib)
 *
 * Connects to the keyboard via HID Feature Reports (Interface 1)
 * and allows configuring key assignments, encoder and LED settings.
 * Configuration is saved to the keyboard's EEPROM — no need to run
 * the app during normal use.
 */

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/Button/Button.h>
#include <UI/Label/Label.h>
#include <UI/Select/Select.h>
#include <IO/HID/HID.h>

#include "protocol.h"

#include <cstdio>
#include <cstring>

/* ================================================================== */
/*  Key / modifier tables                                              */
/* ================================================================== */

struct KeyDef {
    const char* name;
    uint8_t     code;
};

static const KeyDef keyTable[] = {
    /* Letters */
    {"a", 'a'}, {"b", 'b'}, {"c", 'c'}, {"d", 'd'}, {"e", 'e'},
    {"f", 'f'}, {"g", 'g'}, {"h", 'h'}, {"i", 'i'}, {"j", 'j'},
    {"k", 'k'}, {"l", 'l'}, {"m", 'm'}, {"n", 'n'}, {"o", 'o'},
    {"p", 'p'}, {"q", 'q'}, {"r", 'r'}, {"s", 's'}, {"t", 't'},
    {"u", 'u'}, {"v", 'v'}, {"w", 'w'}, {"x", 'x'}, {"y", 'y'},
    {"z", 'z'},
    /* Digits */
    {"0", '0'}, {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'},
    {"5", '5'}, {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'},
    /* Whitespace / special */
    {"Space",     ' '},
    {"Enter",     KC_RETURN},
    {"Escape",    KC_ESC},
    {"Backspace", KC_BACKSPACE},
    {"Tab",       KC_TAB},
    /* Arrows */
    {"Up",    KC_UP_ARROW},
    {"Down",  KC_DOWN_ARROW},
    {"Left",  KC_LEFT_ARROW},
    {"Right", KC_RIGHT_ARROW},
    /* Navigation */
    {"Insert",   KC_INSERT},
    {"Delete",   KC_DELETE},
    {"Home",     KC_HOME},
    {"End",      KC_END},
    {"PageUp",   KC_PAGE_UP},
    {"PageDown", KC_PAGE_DOWN},
    {"CapsLock", KC_CAPS_LOCK},
    /* Function keys */
    {"F1",  KC_F1},  {"F2",  KC_F2},  {"F3",  KC_F3},  {"F4",  KC_F4},
    {"F5",  KC_F5},  {"F6",  KC_F6},  {"F7",  KC_F7},  {"F8",  KC_F8},
    {"F9",  KC_F9},  {"F10", KC_F10}, {"F11", KC_F11}, {"F12", KC_F12},
};

static const int KEY_COUNT = sizeof(keyTable) / sizeof(keyTable[0]);

static const KeyDef modTable[] = {
    {"Brak",        0},
    {"Left Ctrl",   KC_LEFT_CTRL},
    {"Left Shift",  KC_LEFT_SHIFT},
    {"Left Alt",    KC_LEFT_ALT},
    {"Left GUI",    KC_LEFT_GUI},
    {"Right Ctrl",  KC_RIGHT_CTRL},
    {"Right Shift", KC_RIGHT_SHIFT},
    {"Right Alt",   KC_RIGHT_ALT},
    {"Right GUI",   KC_RIGHT_GUI},
};

static const int MOD_COUNT = sizeof(modTable) / sizeof(modTable[0]);

static const KeyDef ledModeTable[] = {
    {"Tecza",    LED_MODE_RAINBOW},
    {"Statyczny", LED_MODE_STATIC},
    {"Oddech",   LED_MODE_BREATHE},
};
static const int LED_MODE_COUNT = sizeof(ledModeTable) / sizeof(ledModeTable[0]);

/* ================================================================== */
/*  Helpers                                                            */
/* ================================================================== */

static int findKeyIndex(uint8_t code) {
    for (int i = 0; i < KEY_COUNT; i++)
        if (keyTable[i].code == code) return i;
    return 0;
}

static int findModIndex(uint8_t code) {
    for (int i = 0; i < MOD_COUNT; i++)
        if (modTable[i].code == code) return i;
    return 0;
}

static int findLedModeIndex(uint8_t code) {
    for (int i = 0; i < LED_MODE_COUNT; i++)
        if (ledModeTable[i].code == code) return i;
    return 0;
}

static int getSelIndex(Select* sel) {
    return (int)SendMessage(sel->getHandle(), CB_GETCURSEL, 0, 0);
}

static void setSelIndex(Select* sel, int idx) {
    SendMessage(sel->getHandle(), CB_SETCURSEL, (WPARAM)idx, 0);
}

static void populateKeys(Select* sel) {
    for (int i = 0; i < KEY_COUNT; i++) sel->addItem(keyTable[i].name);
}

static void populateMods(Select* sel) {
    for (int i = 0; i < MOD_COUNT; i++) sel->addItem(modTable[i].name);
}

static void populateLedModes(Select* sel) {
    for (int i = 0; i < LED_MODE_COUNT; i++) sel->addItem(ledModeTable[i].name);
}

/* ================================================================== */
/*  Globals                                                            */
/* ================================================================== */

static HID             hid;
static SimpleWindow*   window;

/* UI controls */
static Label*  lblStatus;
static Button* btnConnect;

static Select* selKey1;
static Select* selKey2;
static Select* selKey3;
static Select* selEncBtn;

static Select* selCwMod;
static Select* selCwKey;
static Select* selCcwMod;
static Select* selCcwKey;

static Select* selLedMode;
static Select* selBrightness;

static Button* btnRead;
static Button* btnWrite;
static Button* btnReset;
static Button* btnBootloader;

/* ================================================================== */
/*  Read / write helpers                                               */
/* ================================================================== */

static void readFromDevice() {
    if (!hid.isOpen()) {
        lblStatus->setText(L"Najpierw polacz!");
        return;
    }

    uint8_t buf2[CONFIG_REPORT_SIZE];
    uint8_t buf3[CONFIG_REPORT_SIZE];

    if (!hid.getFeatureReport(REPORT_ID_CONFIG1, buf2, CONFIG_REPORT_SIZE) ||
        !hid.getFeatureReport(REPORT_ID_CONFIG2, buf3, CONFIG_REPORT_SIZE)) {
        lblStatus->setText(L"Blad odczytu!");
        return;
    }

    setSelIndex(selKey1,   findKeyIndex(buf2[0]));
    setSelIndex(selKey2,   findKeyIndex(buf2[1]));
    setSelIndex(selKey3,   findKeyIndex(buf2[2]));
    setSelIndex(selEncBtn, findKeyIndex(buf2[3]));
    setSelIndex(selCwMod,  findModIndex(buf2[4]));
    setSelIndex(selCwKey,  findKeyIndex(buf2[5]));
    setSelIndex(selCcwMod, findModIndex(buf2[6]));

    setSelIndex(selCcwKey,     findKeyIndex(buf3[0]));

    /* Brightness — find matching item (0-255 encoded as index into a select
       with items "0","10","20",...,"250","255" — or we store the raw value) */
    {
        int brtIdx = buf3[1] / 10;
        if (brtIdx >= 26) brtIdx = 25;
        setSelIndex(selBrightness, brtIdx);
    }

    setSelIndex(selLedMode, findLedModeIndex(buf3[2]));

    lblStatus->setText(L"Odczytano z urzadzenia");
}

static void writeToDevice() {
    if (!hid.isOpen()) {
        lblStatus->setText(L"Najpierw polacz!");
        return;
    }

    int idx;
    uint8_t buf2[CONFIG_REPORT_SIZE];
    uint8_t buf3[CONFIG_REPORT_SIZE];

    memset(buf2, 0, sizeof(buf2));
    memset(buf3, 0, sizeof(buf3));

    idx = getSelIndex(selKey1);   buf2[0] = (idx >= 0 && idx < KEY_COUNT) ? keyTable[idx].code : DEFAULT_KEY1;
    idx = getSelIndex(selKey2);   buf2[1] = (idx >= 0 && idx < KEY_COUNT) ? keyTable[idx].code : DEFAULT_KEY2;
    idx = getSelIndex(selKey3);   buf2[2] = (idx >= 0 && idx < KEY_COUNT) ? keyTable[idx].code : DEFAULT_KEY3;
    idx = getSelIndex(selEncBtn); buf2[3] = (idx >= 0 && idx < KEY_COUNT) ? keyTable[idx].code : DEFAULT_ENC_BTN;
    idx = getSelIndex(selCwMod);  buf2[4] = (idx >= 0 && idx < MOD_COUNT) ? modTable[idx].code : DEFAULT_CW_MOD;
    idx = getSelIndex(selCwKey);  buf2[5] = (idx >= 0 && idx < KEY_COUNT) ? keyTable[idx].code : DEFAULT_CW_KEY;
    idx = getSelIndex(selCcwMod); buf2[6] = (idx >= 0 && idx < MOD_COUNT) ? modTable[idx].code : DEFAULT_CCW_MOD;

    idx = getSelIndex(selCcwKey); buf3[0] = (idx >= 0 && idx < KEY_COUNT) ? keyTable[idx].code : DEFAULT_CCW_KEY;

    /* Brightness */
    idx = getSelIndex(selBrightness);
    if (idx >= 0 && idx <= 25) {
        buf3[1] = (uint8_t)(idx * 10);
        if (idx == 25) buf3[1] = 255;
    } else {
        buf3[1] = DEFAULT_LED_BRT;
    }

    idx = getSelIndex(selLedMode);
    buf3[2] = (idx >= 0 && idx < LED_MODE_COUNT) ? ledModeTable[idx].code : DEFAULT_LED_MODE;

    if (!hid.setFeatureReport(REPORT_ID_CONFIG1, buf2, CONFIG_REPORT_SIZE) ||
        !hid.setFeatureReport(REPORT_ID_CONFIG2, buf3, CONFIG_REPORT_SIZE)) {
        lblStatus->setText(L"Blad zapisu!");
        return;
    }

    lblStatus->setText(L"Zapisano do urzadzenia (EEPROM)");
}

static void resetDefaults() {
    setSelIndex(selKey1,       findKeyIndex(DEFAULT_KEY1));
    setSelIndex(selKey2,       findKeyIndex(DEFAULT_KEY2));
    setSelIndex(selKey3,       findKeyIndex(DEFAULT_KEY3));
    setSelIndex(selEncBtn,     findKeyIndex(DEFAULT_ENC_BTN));
    setSelIndex(selCwMod,      findModIndex(DEFAULT_CW_MOD));
    setSelIndex(selCwKey,      findKeyIndex(DEFAULT_CW_KEY));
    setSelIndex(selCcwMod,     findModIndex(DEFAULT_CCW_MOD));
    setSelIndex(selCcwKey,     findKeyIndex(DEFAULT_CCW_KEY));
    setSelIndex(selBrightness, DEFAULT_LED_BRT / 10);
    setSelIndex(selLedMode,    findLedModeIndex(DEFAULT_LED_MODE));

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

    window = new SimpleWindow(480, 600, "CH552G Keyboard Configurator", 0);
    window->init();

    int y = 10;
    const int LX    = 10;   /* label X        */
    const int SX    = 180;  /* select X       */
    const int LW    = 160;  /* label width    */
    const int SW    = 200;  /* select width   */
    const int ROW   = 30;   /* row height     */
    const int SELDH = 300;  /* dropdown height */

    /* ---- Connection ---- */
    lblStatus = new Label(LX, y, 260, 22, L"Status: Rozlaczony");
    window->add(lblStatus);

    btnConnect = new Button(360, y, 100, 25, "Polacz", [](Button*) {
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
    y += ROW + 10;

    /* ---- Keys ---- */
    window->add(new Label(LX, y, LW, 20, L"Klawisz 1:"));
    selKey1 = new Select(SX, y, SW, SELDH, "", nullptr);
    window->add(selKey1);
    y += ROW;

    window->add(new Label(LX, y, LW, 20, L"Klawisz 2:"));
    selKey2 = new Select(SX, y, SW, SELDH, "", nullptr);
    window->add(selKey2);
    y += ROW;

    window->add(new Label(LX, y, LW, 20, L"Klawisz 3:"));
    selKey3 = new Select(SX, y, SW, SELDH, "", nullptr);
    window->add(selKey3);
    y += ROW;

    window->add(new Label(LX, y, LW, 20, L"Przycisk enkodera:"));
    selEncBtn = new Select(SX, y, SW, SELDH, "", nullptr);
    window->add(selEncBtn);
    y += ROW + 10;

    /* ---- Encoder CW ---- */
    window->add(new Label(LX, y, SW + LW, 20, L"--- Enkoder: obrot w prawo ---"));
    y += 22;

    window->add(new Label(LX, y, LW, 20, L"Modyfikator:"));
    selCwMod = new Select(SX, y, SW, SELDH, "", nullptr);
    window->add(selCwMod);
    y += ROW;

    window->add(new Label(LX, y, LW, 20, L"Klawisz:"));
    selCwKey = new Select(SX, y, SW, SELDH, "", nullptr);
    window->add(selCwKey);
    y += ROW + 10;

    /* ---- Encoder CCW ---- */
    window->add(new Label(LX, y, SW + LW, 20, L"--- Enkoder: obrot w lewo ---"));
    y += 22;

    window->add(new Label(LX, y, LW, 20, L"Modyfikator:"));
    selCcwMod = new Select(SX, y, SW, SELDH, "", nullptr);
    window->add(selCcwMod);
    y += ROW;

    window->add(new Label(LX, y, LW, 20, L"Klawisz:"));
    selCcwKey = new Select(SX, y, SW, SELDH, "", nullptr);
    window->add(selCcwKey);
    y += ROW + 10;

    /* ---- LED ---- */
    window->add(new Label(LX, y, SW + LW, 20, L"--- Podswietlenie LED ---"));
    y += 22;

    window->add(new Label(LX, y, LW, 20, L"Jasnosc:"));
    selBrightness = new Select(SX, y, SW, SELDH, "", nullptr);
    window->add(selBrightness);
    y += ROW;

    window->add(new Label(LX, y, LW, 20, L"Tryb:"));
    selLedMode = new Select(SX, y, SW, SELDH, "", nullptr);
    window->add(selLedMode);
    y += ROW + 15;

    /* ---- Action buttons ---- */
    btnRead = new Button(LX, y, 140, 30, "Odczytaj", [](Button*) {
        readFromDevice();
    });
    window->add(btnRead);

    btnWrite = new Button(160, y, 140, 30, "Zapisz", [](Button*) {
        writeToDevice();
    });
    window->add(btnWrite);

    btnReset = new Button(310, y, 150, 30, "Domyslne", [](Button*) {
        resetDefaults();
    });
    window->add(btnReset);
    y += 40;

    /* ---- Bootloader button ---- */
    btnBootloader = new Button(LX, y, 220, 30, "Wejdz w Bootloader", [](Button*) {
        if (!hid.isOpen()) {
            lblStatus->setText(L"Najpierw polacz!");
            return;
        }
        uint8_t cmdBuf[CMD_REPORT_SIZE];
        cmdBuf[0] = CMD_BOOTLOADER;
        if (hid.setFeatureReport(REPORT_ID_COMMAND, cmdBuf, CMD_REPORT_SIZE)) {
            lblStatus->setText(L"Bootloader aktywny — mozna wgrac firmware");
            hid.close();
            SetWindowTextA(btnConnect->getHandle(), "Polacz");
        } else {
            lblStatus->setText(L"Blad wysylania komendy bootloadera!");
        }
    });
    window->add(btnBootloader);

    /* ---- Populate dropdowns ---- */
    populateKeys(selKey1);
    populateKeys(selKey2);
    populateKeys(selKey3);
    populateKeys(selEncBtn);
    populateKeys(selCwKey);
    populateKeys(selCcwKey);

    populateMods(selCwMod);
    populateMods(selCcwMod);

    populateLedModes(selLedMode);

    /* Brightness: 0, 10, 20, ... 250, 255 */
    {
        char buf[8];
        for (int v = 0; v <= 250; v += 10) {
            snprintf(buf, sizeof(buf), "%d", v);
            selBrightness->addItem(buf);
        }
        selBrightness->addItem("255");
    }

    /* Set defaults in UI */
    resetDefaults();
}

void loop() {
    /* Nothing to poll — UI is event-driven */
}
