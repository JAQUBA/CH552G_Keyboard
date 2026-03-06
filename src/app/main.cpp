/*
 * main.cpp — CH552G Keyboard Configurator  (Windows, JQB_WindowsLib)
 */

#include <Core.h>
#include <UI/SimpleWindow/SimpleWindow.h>
#include <UI/Button/Button.h>
#include <UI/Label/Label.h>
#include <UI/Select/Select.h>
#include <UI/CheckBox/CheckBox.h>
#include <IO/HID/HID.h>
#include <Util/StringUtils.h>

#include <UI/KeyCapture/KeyCapture.h>

#include "protocol.h"
#include "theme.h"
#include "keymap.h"

#include <cstdio>
#include <cstring>

/* ================================================================== */
/*  VK ↔ firmware helpers                                              */
/* ================================================================== */

static void setKcFromFw(KeyCapture* kc, uint8_t fwMod, uint8_t fwCode) {
    kc->setCombo(fwMod, fwKeyToVk(fwCode));
}

static uint8_t getKcFwMod(KeyCapture* kc) {
    return kc->getCombo().modifiers;
}

static uint8_t getKcFwCode(KeyCapture* kc) {
    return vkToFwKey(kc->getCombo().vkCode);
}

/* ================================================================== */
/*  LED tables                                                         */
/* ================================================================== */

struct LedModeDef { const char* name; uint8_t code; };
static const LedModeDef ledModeTable[] = {
    {"Rainbow",   LED_MODE_RAINBOW},
    {"Static",    LED_MODE_STATIC},
    {"Breathe",   LED_MODE_BREATHE},
};
static const int LED_MODE_COUNT = 3;

struct ColorDef { const char* name; uint8_t r, g, b; };
static const ColorDef colorTable[] = {
    {"White",     255, 255, 255},
    {"Red",       255,   0,   0},
    {"Green",       0, 255,   0},
    {"Blue",        0,   0, 255},
    {"Yellow",    255, 255,   0},
    {"Cyan",        0, 255, 255},
    {"Magenta",   255,   0, 255},
    {"Orange",    255, 128,   0},
    {"Purple",    128,   0, 255},
};
static const int COLOR_COUNT = sizeof(colorTable) / sizeof(colorTable[0]);

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

/* ================================================================== */
/*  Select helpers                                                     */
/* ================================================================== */

static int getSelIndex(Select* sel) {
    return (int)SendMessageW(sel->getHandle(), CB_GETCURSEL, 0, 0);
}
static void setSelIndex(Select* sel, int idx) {
    SendMessageW(sel->getHandle(), CB_SETCURSEL, (WPARAM)idx, 0);
}

/* ================================================================== */
/*  Globals                                                            */
/* ================================================================== */

static HID           hid;
static SimpleWindow* window;

/* Key captures — normal press */
static KeyCapture* kcKey1;
static KeyCapture* kcKey2;
static KeyCapture* kcKey3;
static KeyCapture* kcEncBtn;
static KeyCapture* kcEncCw;
static KeyCapture* kcEncCcw;

/* Key captures — long press */
static KeyCapture* kcKey1Long;
static KeyCapture* kcKey2Long;
static KeyCapture* kcKey3Long;
static KeyCapture* kcEncBtnLong;

/* Status */
static Label*  lblStatus;
static Button* btnConnect;

/* LED controls */
static Select*   selLedMode;
static Select*   selBrightness;
static Select*   selLedColor;
static CheckBox* chkToggle1;
static CheckBox* chkToggle2;
static CheckBox* chkToggle3;

/* Live-preview tracking */
static int  prevModeIdx  = -1;
static int  prevBrtIdx   = -1;
static int  prevColorIdx = -1;
static bool prevTog1 = false, prevTog2 = false, prevTog3 = false;

/* ================================================================== */
/*  Connection helpers                                                 */
/* ================================================================== */

static void updateConnectionUI(bool connected) {
    if (connected) {
        lblStatus->setText(L"Connected");
        lblStatus->setTextColor(CLR_GREEN);
        SetWindowTextW(btnConnect->getHandle(), L"Disconnect");
    } else {
        lblStatus->setText(L"Disconnected");
        lblStatus->setTextColor(CLR_RED);
        SetWindowTextW(btnConnect->getHandle(), L"Connect");
    }
}

static bool tryConnect() {
    if (hid.isOpen()) return true;
    if (hid.findAndOpen()) {
        updateConnectionUI(true);
        return true;
    }
    lblStatus->setText(L"Keyboard not found!");
    lblStatus->setTextColor(CLR_PEACH);
    return false;
}

/* ================================================================== */
/*  LED config send (live preview)                                     */
/* ================================================================== */

static void sendLedConfig() {
    if (!hid.isOpen()) return;

    uint8_t buf[CONFIG_REPORT_SIZE];
    memset(buf, 0, sizeof(buf));

    int idx = getSelIndex(selBrightness);
    buf[0] = (idx >= 0 && idx <= 25)
             ? (uint8_t)((idx == 25) ? 255 : idx * 10)
             : DEFAULT_LED_BRT;

    idx = getSelIndex(selLedMode);
    buf[1] = (idx >= 0 && idx < LED_MODE_COUNT)
             ? ledModeTable[idx].code : DEFAULT_LED_MODE;

    idx = getSelIndex(selLedColor);
    if (idx >= 0 && idx < COLOR_COUNT) {
        buf[2] = colorTable[idx].r;
        buf[3] = colorTable[idx].g;
        buf[4] = colorTable[idx].b;
    } else {
        buf[2] = DEFAULT_LED_R;  buf[3] = DEFAULT_LED_G;  buf[4] = DEFAULT_LED_B;
    }

    if (chkToggle1->isChecked()) buf[5] |= 0x01;
    if (chkToggle2->isChecked()) buf[5] |= 0x02;
    if (chkToggle3->isChecked()) buf[5] |= 0x04;

    hid.setFeatureReport(REPORT_ID_LED, buf, CONFIG_REPORT_SIZE);
}

/* ================================================================== */
/*  Read / Write / Reset                                               */
/* ================================================================== */

static void saveLedSnapshot() {
    prevModeIdx  = getSelIndex(selLedMode);
    prevBrtIdx   = getSelIndex(selBrightness);
    prevColorIdx = getSelIndex(selLedColor);
    prevTog1 = chkToggle1->isChecked();
    prevTog2 = chkToggle2->isChecked();
    prevTog3 = chkToggle3->isChecked();
}

static void readFromDevice() {
    if (!tryConnect()) return;

    uint8_t b2[CONFIG_REPORT_SIZE], b3[CONFIG_REPORT_SIZE], b5[CONFIG_REPORT_SIZE];
    uint8_t b6[CONFIG_REPORT_SIZE], b7[CONFIG_REPORT_SIZE];
    if (!hid.getFeatureReport(REPORT_ID_KEYS,    b2, CONFIG_REPORT_SIZE) ||
        !hid.getFeatureReport(REPORT_ID_ENCODER, b3, CONFIG_REPORT_SIZE) ||
        !hid.getFeatureReport(REPORT_ID_LED,     b5, CONFIG_REPORT_SIZE) ||
        !hid.getFeatureReport(REPORT_ID_LONG_KEYS, b6, CONFIG_REPORT_SIZE) ||
        !hid.getFeatureReport(REPORT_ID_LONG_ENC,  b7, CONFIG_REPORT_SIZE)) {
        lblStatus->setText(L"Read error!");
        lblStatus->setTextColor(CLR_RED);
        return;
    }

    setKcFromFw(kcKey1,   b2[0], b2[1]);
    setKcFromFw(kcKey2,   b2[2], b2[3]);
    setKcFromFw(kcKey3,   b2[4], b2[5]);
    setKcFromFw(kcEncBtn, b2[6], b3[0]);
    setKcFromFw(kcEncCw,  b3[1], b3[2]);
    setKcFromFw(kcEncCcw, b3[3], b3[4]);

    /* Long press */
    setKcFromFw(kcKey1Long,   b6[0], b6[1]);
    setKcFromFw(kcKey2Long,   b6[2], b6[3]);
    setKcFromFw(kcKey3Long,   b6[4], b6[5]);
    setKcFromFw(kcEncBtnLong, b6[6], b7[0]);

    int brtIdx = b5[0] / 10;
    if (brtIdx > 25) brtIdx = 25;
    setSelIndex(selBrightness, brtIdx);
    setSelIndex(selLedMode,  findLedModeIndex(b5[1]));
    setSelIndex(selLedColor, findColorIndex(b5[2], b5[3], b5[4]));

    chkToggle1->setChecked((b5[5] & 0x01) != 0);
    chkToggle2->setChecked((b5[5] & 0x02) != 0);
    chkToggle3->setChecked((b5[5] & 0x04) != 0);

    saveLedSnapshot();

    lblStatus->setText(L"Settings loaded");
    lblStatus->setTextColor(CLR_GREEN);
}

static void writeToDevice() {
    if (!tryConnect()) return;

    uint8_t b2[CONFIG_REPORT_SIZE], b3[CONFIG_REPORT_SIZE], b5[CONFIG_REPORT_SIZE];
    uint8_t b6[CONFIG_REPORT_SIZE], b7[CONFIG_REPORT_SIZE];
    memset(b2, 0, sizeof(b2));
    memset(b3, 0, sizeof(b3));
    memset(b5, 0, sizeof(b5));
    memset(b6, 0, sizeof(b6));
    memset(b7, 0, sizeof(b7));

    b2[0] = getKcFwMod(kcKey1);   b2[1] = getKcFwCode(kcKey1);
    b2[2] = getKcFwMod(kcKey2);   b2[3] = getKcFwCode(kcKey2);
    b2[4] = getKcFwMod(kcKey3);   b2[5] = getKcFwCode(kcKey3);
    b2[6] = getKcFwMod(kcEncBtn);
    b3[0] = getKcFwCode(kcEncBtn);
    b3[1] = getKcFwMod(kcEncCw);  b3[2] = getKcFwCode(kcEncCw);
    b3[3] = getKcFwMod(kcEncCcw); b3[4] = getKcFwCode(kcEncCcw);

    /* Long press */
    b6[0] = getKcFwMod(kcKey1Long);   b6[1] = getKcFwCode(kcKey1Long);
    b6[2] = getKcFwMod(kcKey2Long);   b6[3] = getKcFwCode(kcKey2Long);
    b6[4] = getKcFwMod(kcKey3Long);   b6[5] = getKcFwCode(kcKey3Long);
    b6[6] = getKcFwMod(kcEncBtnLong);
    b7[0] = getKcFwCode(kcEncBtnLong);

    int idx = getSelIndex(selBrightness);
    b5[0] = (idx >= 0 && idx <= 25)
            ? (uint8_t)((idx == 25) ? 255 : idx * 10)
            : DEFAULT_LED_BRT;

    idx = getSelIndex(selLedMode);
    b5[1] = (idx >= 0 && idx < LED_MODE_COUNT)
            ? ledModeTable[idx].code : DEFAULT_LED_MODE;

    idx = getSelIndex(selLedColor);
    if (idx >= 0 && idx < COLOR_COUNT) {
        b5[2] = colorTable[idx].r;
        b5[3] = colorTable[idx].g;
        b5[4] = colorTable[idx].b;
    } else {
        b5[2] = DEFAULT_LED_R;  b5[3] = DEFAULT_LED_G;  b5[4] = DEFAULT_LED_B;
    }

    if (chkToggle1->isChecked()) b5[5] |= 0x01;
    if (chkToggle2->isChecked()) b5[5] |= 0x02;
    if (chkToggle3->isChecked()) b5[5] |= 0x04;

    if (!hid.setFeatureReport(REPORT_ID_KEYS,    b2, CONFIG_REPORT_SIZE) ||
        !hid.setFeatureReport(REPORT_ID_ENCODER, b3, CONFIG_REPORT_SIZE) ||
        !hid.setFeatureReport(REPORT_ID_LED,     b5, CONFIG_REPORT_SIZE) ||
        !hid.setFeatureReport(REPORT_ID_LONG_KEYS, b6, CONFIG_REPORT_SIZE) ||
        !hid.setFeatureReport(REPORT_ID_LONG_ENC,  b7, CONFIG_REPORT_SIZE)) {
        lblStatus->setText(L"Write error!");
        lblStatus->setTextColor(CLR_RED);
        return;
    }

    /* Persist to EEPROM with a single save command */
    uint8_t cmd = CMD_SAVE;
    if (!hid.setFeatureReport(REPORT_ID_COMMAND, &cmd, CMD_REPORT_SIZE)) {
        lblStatus->setText(L"Save error!");
        lblStatus->setTextColor(CLR_RED);
        return;
    }

    saveLedSnapshot();
    lblStatus->setText(L"Saved to EEPROM");
    lblStatus->setTextColor(CLR_GREEN);
}

static void resetDefaults() {
    setKcFromFw(kcKey1,   DEFAULT_KEY1_MOD,    DEFAULT_KEY1);
    setKcFromFw(kcKey2,   DEFAULT_KEY2_MOD,    DEFAULT_KEY2);
    setKcFromFw(kcKey3,   DEFAULT_KEY3_MOD,    DEFAULT_KEY3);
    setKcFromFw(kcEncBtn, DEFAULT_ENC_BTN_MOD, DEFAULT_ENC_BTN);
    setKcFromFw(kcEncCw,  DEFAULT_CW_MOD,      DEFAULT_CW_KEY);
    setKcFromFw(kcEncCcw, DEFAULT_CCW_MOD,     DEFAULT_CCW_KEY);

    /* Long press defaults (disabled) */
    setKcFromFw(kcKey1Long,   DEFAULT_KEY1_LONG_MOD,    DEFAULT_KEY1_LONG);
    setKcFromFw(kcKey2Long,   DEFAULT_KEY2_LONG_MOD,    DEFAULT_KEY2_LONG);
    setKcFromFw(kcKey3Long,   DEFAULT_KEY3_LONG_MOD,    DEFAULT_KEY3_LONG);
    setKcFromFw(kcEncBtnLong, DEFAULT_ENC_BTN_LONG_MOD, DEFAULT_ENC_BTN_LONG);

    setSelIndex(selBrightness, DEFAULT_LED_BRT / 10);
    setSelIndex(selLedMode,    findLedModeIndex(DEFAULT_LED_MODE));
    setSelIndex(selLedColor,   findColorIndex(DEFAULT_LED_R, DEFAULT_LED_G, DEFAULT_LED_B));

    chkToggle1->setChecked((DEFAULT_LED_TOGGLE & 0x01) != 0);
    chkToggle2->setChecked((DEFAULT_LED_TOGGLE & 0x02) != 0);
    chkToggle3->setChecked((DEFAULT_LED_TOGGLE & 0x04) != 0);

    lblStatus->setText(L"Defaults restored");
    lblStatus->setTextColor(CLR_PEACH);
}

/* ================================================================== */
/*  UI helpers                                                         */
/* ================================================================== */

static Label* makeLabel(int x, int y, int w, int h, const wchar_t* text,
                         COLORREF color = CLR_TEXT, bool bold = false,
                         int fontSize = 13) {
    Label* lbl = new Label(x, y, w, h, text);
    window->add(lbl);
    lbl->setTextColor(color);
    lbl->setFont(L"Segoe UI", fontSize, bold);
    return lbl;
}

static void makeSeparator(int y) {
    Label* sep = new Label(20, y, 520, 1, L"");
    window->add(sep);
    sep->setBackColor(CLR_SEPARATOR);
}

static Button* makeActionBtn(int x, int y, int w, int h, const char* text,
                              COLORREF bg, COLORREF hover,
                              std::function<void(Button*)> onClick) {
    Button* btn = new Button(x, y, w, h, text, onClick);
    btn->setBackColor(bg);
    btn->setTextColor(CLR_TEXT);
    btn->setHoverColor(hover);
    btn->setFont(L"Segoe UI", 13, true);
    window->add(btn);
    return btn;
}

static KeyCapture* makeKeyCapture(int x, int y, int w, int h,
                                  uint8_t fwMod, uint8_t fwCode) {
    KeyCapture* kc = new KeyCapture(x, y, w, h);
    kc->setBackColor(CLR_SURFACE2);
    kc->setTextColor(CLR_TEXT);
    kc->setActiveColor(CLR_BTN_LISTEN);
    kc->setActiveBorderColor(CLR_ACCENT);
    kc->setBorderColor(CLR_SEPARATOR);
    window->add(kc);
    setKcFromFw(kc, fwMod, fwCode);
    return kc;
}

/* ================================================================== */
/*  init / setup / loop                                                */
/* ================================================================== */

void init() {}

void setup() {
    /* ── HID init ─────────────────────────────────────────────── */

    hid.init();
    hid.setVidPid(KB_VID, KB_PID);
    hid.setUsage(VENDOR_USAGE_PAGE, VENDOR_USAGE);
    hid.setFeatureReportSize(CONFIG_REPORT_SIZE);

    /* ── Window ───────────────────────────────────────────────── */

    window = new SimpleWindow(620, 570, "CH552G Keyboard Configurator", 1);
    window->init();
    window->setBackgroundColor(CLR_BG);
    window->setTextColor(CLR_TEXT);

    /* ── Layout constants ─────────────────────────────────────── */

    const int LX    = 24;
    const int LBL_W = 140;
    const int KC_X  = 170;
    const int KC_W  = 160;
    const int KC_H  = 30;
    const int KC_LP_X = KC_X + KC_W + 10;  /* long-press column */

    int y = 14;

    /* ── Header ───────────────────────────────────────────────── */

    makeLabel(LX, y, 340, 22,
              L"CH552G Keyboard Configurator",
              CLR_ACCENT, true, 15);

    lblStatus = makeLabel(LX, y + 26, 280, 16,
                          L"Disconnected", CLR_RED, false, 12);

    btnConnect = makeActionBtn(596 - 120, y + 4, 110, 34,
        "Connect", CLR_SURFACE2, CLR_BTN_HOVER,
        [](Button*) {
            if (hid.isOpen()) {
                hid.close();
                updateConnectionUI(false);
            } else {
                if (hid.findAndOpen())
                    updateConnectionUI(true);
                else {
                    lblStatus->setText(L"Keyboard not found!");
                    lblStatus->setTextColor(CLR_PEACH);
                }
            }
        });

    y = 56;
    makeSeparator(y);

    /* ── KLAWISZE ─────────────────────────────────────────────── */

    y += 10;
    makeLabel(LX, y, 200, 18, L"KEYS", CLR_ACCENT, true, 12);
    y += 4;
    makeLabel(LX, y + 16, 560, 14,
        L"Click a button, then press a key combo (e.g. Ctrl+F6). Media keys supported.",
        CLR_TEXT_DIM, false, 11);
    y += 30;

    /* Column headers */
    makeLabel(KC_X, y, KC_W, 14, L"Press", CLR_TEXT_DIM, false, 11);
    makeLabel(KC_LP_X, y, KC_W, 14, L"Long Press", CLR_TEXT_DIM, false, 11);
    y += 18;

    makeLabel(LX, y + 5, LBL_W, 18, L"Key 1:", CLR_SUBTEXT);
    kcKey1 = makeKeyCapture(KC_X, y, KC_W, KC_H,
                            DEFAULT_KEY1_MOD, DEFAULT_KEY1);
    kcKey1Long = makeKeyCapture(KC_LP_X, y, KC_W, KC_H,
                                DEFAULT_KEY1_LONG_MOD, DEFAULT_KEY1_LONG);
    y += 36;

    makeLabel(LX, y + 5, LBL_W, 18, L"Key 2:", CLR_SUBTEXT);
    kcKey2 = makeKeyCapture(KC_X, y, KC_W, KC_H,
                            DEFAULT_KEY2_MOD, DEFAULT_KEY2);
    kcKey2Long = makeKeyCapture(KC_LP_X, y, KC_W, KC_H,
                                DEFAULT_KEY2_LONG_MOD, DEFAULT_KEY2_LONG);
    y += 36;

    makeLabel(LX, y + 5, LBL_W, 18, L"Key 3:", CLR_SUBTEXT);
    kcKey3 = makeKeyCapture(KC_X, y, KC_W, KC_H,
                            DEFAULT_KEY3_MOD, DEFAULT_KEY3);
    kcKey3Long = makeKeyCapture(KC_LP_X, y, KC_W, KC_H,
                                DEFAULT_KEY3_LONG_MOD, DEFAULT_KEY3_LONG);

    /* ── ENKODER ──────────────────────────────────────────────── */

    y += 44;
    makeSeparator(y);
    y += 10;
    makeLabel(LX, y, 200, 18, L"ENCODER", CLR_MAUVE, true, 12);
    y += 24;

    makeLabel(LX, y + 5, LBL_W, 18, L"Button:", CLR_SUBTEXT);
    kcEncBtn = makeKeyCapture(KC_X, y, KC_W, KC_H,
                              DEFAULT_ENC_BTN_MOD, DEFAULT_ENC_BTN);
    kcEncBtnLong = makeKeyCapture(KC_LP_X, y, KC_W, KC_H,
                                  DEFAULT_ENC_BTN_LONG_MOD, DEFAULT_ENC_BTN_LONG);
    y += 36;

    makeLabel(LX, y + 5, LBL_W, 18, L"Rotate CW:", CLR_SUBTEXT);
    kcEncCw = makeKeyCapture(KC_X, y, KC_W, KC_H,
                             DEFAULT_CW_MOD, DEFAULT_CW_KEY);
    y += 36;

    makeLabel(LX, y + 5, LBL_W, 18, L"Rotate CCW:", CLR_SUBTEXT);
    kcEncCcw = makeKeyCapture(KC_X, y, KC_W, KC_H,
                              DEFAULT_CCW_MOD, DEFAULT_CCW_KEY);

    /* ── PODSWIETLENIE LED ────────────────────────────────────── */

    y += 44;
    makeSeparator(y);
    y += 10;
    makeLabel(LX, y, 320, 18, L"LED", CLR_GREEN, true, 12);
    y += 26;

    const int SEL_W = 110;
    const int SEL_H = 300;

    int sx = LX;
    makeLabel(sx, y + 3, 46, 18, L"Mode:", CLR_SUBTEXT);
    sx += 46;
    selLedMode = new Select(sx, y, SEL_W, SEL_H, "", nullptr);
    selLedMode->create(window->getHandle());
    sx += SEL_W + 12;

    makeLabel(sx, y + 3, 80, 18, L"Brightness:", CLR_SUBTEXT);
    sx += 80;
    selBrightness = new Select(sx, y, 66, SEL_H, "", nullptr);
    selBrightness->create(window->getHandle());
    sx += 66 + 12;

    makeLabel(sx, y + 3, 46, 18, L"Color:", CLR_SUBTEXT);
    sx += 46;
    selLedColor = new Select(sx, y, SEL_W, SEL_H, "", nullptr);
    selLedColor->create(window->getHandle());

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

    /* Toggle row */
    y += 34;
    makeLabel(LX, y + 2, 55, 18, L"Toggle:", CLR_TEXT_DIM);
    int tx = LX + 60;
    chkToggle1 = new CheckBox(tx, y, 130, 22, "LED 1", false);
    window->add(chkToggle1);
    tx += 138;
    chkToggle2 = new CheckBox(tx, y, 130, 22, "LED 2", false);
    window->add(chkToggle2);
    tx += 138;
    chkToggle3 = new CheckBox(tx, y, 130, 22, "LED 3", false);
    window->add(chkToggle3);

    /* ── Action buttons ───────────────────────────────────────── */

    y += 30;
    makeSeparator(y);
    y += 14;

    const int BW = 115, BH = 36, BG = 12;
    int bx = LX;

    Button* bRead = makeActionBtn(bx, y, BW, BH, "Read",
        CLR_ACCENT, CLR_ACCENT_H, [](Button*) { readFromDevice(); });
    bRead->setTextColor(CLR_BG);
    bx += BW + BG;

    Button* bWrite = makeActionBtn(bx, y, BW, BH, "Write",
        CLR_GREEN, RGB(186, 247, 181), [](Button*) { writeToDevice(); });
    bWrite->setTextColor(CLR_BG);
    bx += BW + BG;

    makeActionBtn(bx, y, BW, BH, "Defaults",
        CLR_SURFACE2, CLR_BTN_HOVER, [](Button*) { resetDefaults(); });
    bx += BW + BG;

    makeActionBtn(bx, y, BW, BH, "Bootloader",
        CLR_SURFACE2, RGB(80, 50, 60), [](Button*) {
            if (!tryConnect()) return;
            uint8_t cmd[CMD_REPORT_SIZE];
            cmd[0] = CMD_BOOTLOADER;
            hid.setFeatureReport(REPORT_ID_COMMAND, cmd, CMD_REPORT_SIZE);
            hid.close();
            updateConnectionUI(false);
            lblStatus->setText(L"Bootloader active");
            lblStatus->setTextColor(CLR_PEACH);
        });

    /* ── Auto-save on close ──────────────────────────────────────── */

    window->onClose([]() {
        if (hid.isOpen()) {
            writeToDevice();
            hid.close();          // prevent loop() from overwriting with 0s
        }
    });

    /* ── Startup: defaults → try connect → read ───────────────── */

    resetDefaults();
    saveLedSnapshot();

    if (hid.findAndOpen()) {
        updateConnectionUI(true);
        readFromDevice();
    }
}

/* ================================================================== */
/*  loop — live LED preview                                            */
/* ================================================================== */

void loop() {
    if (!hid.isOpen() || !selLedMode || !selBrightness || !selLedColor)
        return;

    int curMode  = getSelIndex(selLedMode);
    int curBrt   = getSelIndex(selBrightness);
    int curColor = getSelIndex(selLedColor);
    bool curT1   = chkToggle1->isChecked();
    bool curT2   = chkToggle2->isChecked();
    bool curT3   = chkToggle3->isChecked();

    if (curMode != prevModeIdx || curBrt != prevBrtIdx ||
        curColor != prevColorIdx || curT1 != prevTog1 ||
        curT2 != prevTog2 || curT3 != prevTog3) {
        prevModeIdx  = curMode;
        prevBrtIdx   = curBrt;
        prevColorIdx = curColor;
        prevTog1 = curT1;
        prevTog2 = curT2;
        prevTog3 = curT3;
        sendLedConfig();
    }
}
