# CH552G Keyboard — Copilot Instructions

## Projekt

Firmware dla 3-klawiszowej klawiatury HID z enkoderem obrotowym i podświetleniem NeoPixel,
opartej na mikrokontrolerze **WCH CH552G** (architektura MCS51 / 8051).  
Projekt zawiera dwa środowiska PlatformIO:
- **board** — firmware CH552G (SDCC, język C)
- **app** — aplikacja konfiguracyjna Windows (MinGW GCC, C++17, biblioteka JQB_WindowsLib)

Budowany za pomocą **PlatformIO** z lokalną platformą (`platform/ch552`) i frameworkiem **ch55xduino**.

---

## Architektura i ograniczenia

| Parametr | Wartość |
|---|---|
| MCU | WCH CH552G (MCS51 / 8051) |
| Zegar | 24 MHz |
| Flash | 14 336 B (16 KB − 2 KB bootloader) |
| XRAM | 876 B (1 KB − 148 B USB endpoints) |
| RAM wewnętrzny | 256 B |
| DataFlash (EEPROM) | 128 B |
| Kompilator | **SDCC** (Small Device C Compiler) |
| Język firmware | **Tylko C** — SDCC dla MCS51 **nie obsługuje C++** |
| Model pamięci | `--model-large --int-long-reent` |

### Krytyczne zasady (firmware)

- **Nie używaj C++** — brak klas, szablonów, `new`, `delete`, przestrzeni nazw, referencji `&`.
- Pliki źródłowe firmware muszą mieć rozszerzenie **`.c`** (nie `.cpp`).
- Pamięć jest ekstremalnie ograniczona — unikaj dużych buforów i rekurencji.
- Zmienne globalne domyślnie trafiają do XRAM (`__xdata`). Dla krytycznych czasowo danych użyj `__data` (wewnętrzny RAM, max 256 B).
- **Nie używaj `printf` / `sprintf`** — zbyt duże na 8051. Używaj ręcznej konwersji.
- **USB Serial (`USBSerial_*`) jest niedostępny** — cały CDC jest wyłączony (`-DUSER_USB_RAM=148`).
- Przy mnożeniu 8-bitowych wartości SDCC promuje do `int` (16-bit signed) — **castuj na `(uint16_t)`** aby uniknąć overflow (np. `((uint16_t)s * remainder) >> 8`).

---

## Composite USB HID

Firmware działa jako **composite USB HID** z dwoma interfejsami:

| Interfejs | Funkcja | Endpoint | Deskryptor |
|---|---|---|---|
| Interface 0 | HID Keyboard | EP1 IN (8 B) | Boot Keyboard (6KRO) |
| Interface 1 | HID Vendor (config) | EP0 (control) | Feature Reports |

- **VID/PID:** `0x1209` / `0xC55D`
- `-DUSER_USB_RAM=148` wyłącza domyślny CDC i włącza własny stos USB.
- Implementacja w `src/board/userUsbHidKeyboard/`:
  - `USBconstant.c/h` — deskryptory USB, VID/PID, Report Descriptory obu interfejsów
  - `USBhandler.c/h` — obsługa przerwań USB (EP0 SETUP/IN/OUT, EP1 IN, EP2 IN)
  - `USBHIDKeyboard.c/h` — API klawiatury: `Keyboard_press()`, `Keyboard_release()`, `Keyboard_write()`
- Inicjalizacja: `USBInit()` w `setup()`.

### Feature Reports (Interface 1 — Vendor)

Konfiguracja klawiatury odbywa się przez **HID Feature Reports** na EP0 (control transfer).

> **WAŻNE:** W fazie DATA transferu kontrolnego (GET_REPORT / SET_REPORT) **pierwszy bajt to Report ID**. Dane zaczynają się od `Ep0Buffer[1]`, nie `Ep0Buffer[0]`.

| Report ID | Rozmiar danych | Kierunek | Opis |
|---|---|---|---|
| 1 | 1 B | IN (EP2) | Dummy Input Report (wymagany przez Windows) |
| 2 | 7 B | GET/SET Feature | Mapowanie klawiszy (mod+key) |
| 3 | 7 B | GET/SET Feature | Mapowanie enkodera (mod+key) |
| 4 | 1 B | SET Feature | Komenda (0x42 = bootloader) |
| 5 | 7 B | GET/SET Feature | Konfiguracja LED |

#### Report ID 2 — mapowanie klawiszy (7 bajtów danych):

| Offset | Pole | Opis |
|---|---|---|
| 0 | `key1_mod` | Maska modyfikatorów klawisza 1 (bitmask) |
| 1 | `key1_code` | Kod klawisza 1 |
| 2 | `key2_mod` | Maska modyfikatorów klawisza 2 (bitmask) |
| 3 | `key2_code` | Kod klawisza 2 |
| 4 | `key3_mod` | Maska modyfikatorów klawisza 3 (bitmask) |
| 5 | `key3_code` | Kod klawisza 3 |
| 6 | `enc_btn_mod` | Maska modyfikatorów przycisku enkodera (bitmask) |

#### Report ID 3 — mapowanie enkodera (7 bajtów danych):

| Offset | Pole | Opis |
|---|---|---|
| 0 | `enc_btn_code` | Kod przycisku enkodera |
| 1 | `enc_cw_mod` | Maska modyfikatorów obrotu CW (bitmask) |
| 2 | `enc_cw_key` | Klawisz: obrót CW |
| 3 | `enc_ccw_mod` | Maska modyfikatorów obrotu CCW (bitmask) |
| 4 | `enc_ccw_key` | Klawisz: obrót CCW |
| 5 | (zarezerwowane) | 0 |
| 6 | (zarezerwowane) | 0 |

#### Report ID 4 — komenda (1 bajt danych):

| Wartość | Akcja |
|---|---|
| `0x42` (`CMD_BOOTLOADER`) | Wejście w bootloader USB |

#### Report ID 5 — konfiguracja LED (7 bajtów danych):

| Offset | Pole | Opis |
|---|---|---|
| 0 | `led_brightness` | Jasność LED (0–255) |
| 1 | `led_mode` | Tryb LED (0=rainbow, 1=static, 2=breathe) |
| 2 | `led_r` | Kolor R (0–255) |
| 3 | `led_g` | Kolor G (0–255) |
| 4 | `led_b` | Kolor B (0–255) |
| 5 | `led_toggle` | Maska toggle (bit0=key1, bit1=key2, bit2=key3) |
| 6 | (zarezerwowane) | 0 |

### Modifier bitmask

Każdy klawisz i funkcja enkodera ma przypisaną **maskę modyfikatorów** (`uint8_t`). Bity odpowiadają bajtowi modyfikatorów HID Keyboard:

| Bit | Stała | Modyfikator |
|-----|-------|-------------|
| 0 | `MOD_BIT_LCTRL` (0x01) | Left Ctrl |
| 1 | `MOD_BIT_LSHIFT` (0x02) | Left Shift |
| 2 | `MOD_BIT_LALT` (0x04) | Left Alt |
| 3 | `MOD_BIT_LGUI` (0x08) | Left GUI (Win) |
| 4 | `MOD_BIT_RCTRL` (0x10) | Right Ctrl |
| 5 | `MOD_BIT_RSHIFT` (0x20) | Right Shift |
| 6 | `MOD_BIT_RALT` (0x40) | Right Alt |
| 7 | `MOD_BIT_RGUI` (0x80) | Right GUI (Win) |

Przykład: Ctrl+Alt+F6 → `mod = MOD_BIT_LCTRL | MOD_BIT_LALT` (0x05), `key = 0xC3` (F6).

Firmware wysyła modyfikatory przez `press_mod_bits()` / `release_mod_bits()`, które iterują bity 0–7 i wywołują `Keyboard_press(0x80 + i)` / `Keyboard_release(0x80 + i)`.

### Vendor HID Usage (Interface 1)

```
Usage Page: 0xFF00 (Vendor Defined)
Usage:      0x01
```

Aplikacja Windows łączy się z Interface 1 podając te Usage.

---

## Konfiguracja — EEPROM (DataFlash)

Mapowanie klawiszy, enkodera i LED **nie jest hardkodowane w `#define`** — jest przechowywane w **DataFlash (EEPROM)** CH552 i ładowane przy starcie.

### Struct `KeyboardConfig` (`config.h`)

```c
typedef struct {
    uint8_t key1_mod;        // Maska modyfikatorów klawisza 1
    uint8_t key1_code;       // Kod klawisza 1
    uint8_t key2_mod;        // Maska modyfikatorów klawisza 2
    uint8_t key2_code;       // Kod klawisza 2
    uint8_t key3_mod;        // Maska modyfikatorów klawisza 3
    uint8_t key3_code;       // Kod klawisza 3
    uint8_t enc_btn_mod;     // Maska modyfikatorów przycisku enkodera
    uint8_t enc_btn_code;    // Kod przycisku enkodera
    uint8_t enc_cw_mod;      // Maska modyfikatorów CW (bitmask)
    uint8_t enc_cw_key;      // Klawisz CW
    uint8_t enc_ccw_mod;     // Maska modyfikatorów CCW (bitmask)
    uint8_t enc_ccw_key;     // Klawisz CCW
    uint8_t led_brightness;  // Jasność (0–255)
    uint8_t led_mode;        // Tryb LED (0/1/2)
    uint8_t led_r, led_g, led_b; // Kolor statyczny
    uint8_t led_toggle;      // Maska toggle per-key
} KeyboardConfig;

extern __xdata KeyboardConfig g_config;
```

### API (`config.c`)

| Funkcja | Opis |
|---|---|
| `config_load()` | Ładuje z EEPROM; jeśli brak magic byte → `config_reset()` |
| `config_save()` | Zapisuje do EEPROM (18 bajtów + magic) |
| `config_reset()` | Przywraca domyślne i zapisuje |
| `config_pack_report2(buf)` | Pakuje g_config → bufor Report ID 2 (7 B) — mapowanie klawiszy |
| `config_unpack_report2(buf)` | Rozpakowuje bufor Report ID 2 → g_config |
| `config_pack_report3(buf)` | Pakuje g_config → bufor Report ID 3 (7 B) — mapowanie enkodera |
| `config_unpack_report3(buf)` | Rozpakowuje bufor Report ID 3 → g_config |
| `config_pack_report5(buf)` | Pakuje g_config → bufor Report ID 5 (7 B) — konfiguracja LED |
| `config_unpack_report5(buf)` | Rozpakowuje bufor Report ID 5 → g_config |

### Layout EEPROM (DataFlash)

| Adres | Pole | Domyślna wartość |
|---|---|---|
| 0 | Magic (`0xA6`) | — |
| 1 | key1_mod | `0x00` (brak) |
| 2 | key1_code | `0x62` ('b') |
| 3 | key2_mod | `0x00` (brak) |
| 4 | key2_code | `0x66` ('f') |
| 5 | key3_mod | `0x00` (brak) |
| 6 | key3_code | `0x65` ('e') |
| 7 | enc_btn_mod | `0x00` (brak) |
| 8 | enc_btn_code | `0xB0` (Enter) |
| 9 | enc_cw_mod | `0x02` (LShift bitmask) |
| 10 | enc_cw_key | `0xDA` (Up) |
| 11 | enc_ccw_mod | `0x02` (LShift bitmask) |
| 12 | enc_ccw_key | `0xD9` (Down) |
| 13 | led_brightness | `40` |
| 14 | led_mode | `0` (rainbow) |
| 15 | led_r | `255` |
| 16 | led_g | `255` |
| 17 | led_b | `255` |
| 18 | led_toggle | `0x00` |

> **EEPROM_CONFIG_SIZE = 19.** Magic zmieniony z 0xA5 na 0xA6 po dodaniu pól modyfikatorów — urządzenie z starym firmware automatycznie zresetuje konfigurację do domyślnych przy pierwszym uruchomieniu.

### Protokół współdzielony — `src/shared/protocol.h`

Plik `protocol.h` jest includowany zarówno przez firmware (C) jak i aplikację (C++). Definiuje:
- VID/PID, Usage Page/Usage
- Report ID i rozmiary (`REPORT_ID_KEYS=2`, `REPORT_ID_ENCODER=3`, `REPORT_ID_COMMAND=4`, `REPORT_ID_LED=5`, `CONFIG_REPORT_SIZE=7`)
- Modifier bitmask: `MOD_BIT_LCTRL` (0x01) – `MOD_BIT_RGUI` (0x80)
- Layout EEPROM (adresy i magic byte 0xA6)
- Tryby LED (`LED_MODE_RAINBOW=0`, `LED_MODE_STATIC=1`, `LED_MODE_BREATHE=2`)
- Wartości domyślne z modyfikatorami (`DEFAULT_KEY1_MOD`, `DEFAULT_KEY1`, `DEFAULT_CW_MOD`, itp.)
- Kody klawiszy (`KC_LEFT_SHIFT=0x81`, `KC_UP_ARROW=0xDA`, itp.) — duplikat z `USBHIDKeyboard.h` dla aplikacji

---

## Mapowanie pinów — hardware klawiatury

ch55xduino używa schematu: **`NumerPortu * 10 + NumerPinu`**

Piny są stałe (wynikają z PCB) i zdefiniowane jako `#define` w `main.c`:

| Funkcja | Pin MCU | Numer w kodzie | Konfiguracja |
|---|---|---|---|
| Klawisz 1 | P1.1 | `11` | `INPUT_PULLUP`, aktywny LOW |
| Klawisz 2 | P1.7 | `17` | `INPUT_PULLUP`, aktywny LOW |
| Klawisz 3 | P1.6 | `16` | `INPUT_PULLUP`, aktywny LOW |
| Enkoder kanał A | P3.0 | `30` | `INPUT_PULLUP` |
| Enkoder kanał B | P3.1 | `31` | `INPUT_PULLUP` |
| Przycisk enkodera | P3.3 | `33` | `INPUT_PULLUP`, aktywny LOW |
| NeoPixel (3× WS2812) | P3.4 | `34` | `OUTPUT`, GRB format |

### Wszystkie piny CH552G

| Pin MCU | Numer w kodzie | Uwagi |
|---|---|---|
| P1.0 | `10` | GPIO |
| P1.1 | `11` | GPIO / analog ch0 |
| P1.4 | `14` | GPIO / analog ch1 |
| P1.5 | `15` | GPIO / analog ch2 / PWM1 |
| P1.6 | `16` | GPIO |
| P1.7 | `17` | GPIO |
| P3.0 | `30` | GPIO / PWM1_ |
| P3.1 | `31` | GPIO / PWM2_ |
| P3.2 | `32` | GPIO / analog ch3 |
| P3.3 | `33` | GPIO |
| P3.4 | `34` | GPIO / PWM2 |

> **Uwaga:** P3.6 i P3.7 to linie USB (D+ / D−) — **nigdy** nie używać jako GPIO!

---

## NeoPixel (WS2812) i tryby LED

- 3 diody WS2812 na pinie P3.4, format **GRB**.
- Biblioteka: `WS2812` z ch55xduino (włączona przez `build_src_filter` w `platformio.ini`).
- Bufor: `__xdata uint8_t led_data[NUM_LEDS * 3]` (9 bajtów XRAM).
- Animacja throttlowana przez `led_frame_cnt` — nie każda iteracja `loop()` aktualizuje stan.

### Funkcje

- `set_pixel_for_GRB_LED(buffer, index, r, g, b)` — ustaw kolor piksela
- `neopixel_show_P3_4(buffer, length)` — wyślij dane do LED (dedykowana do pinu P3.4)
- `hsv_to_rgb(h, s, v, &r, &g, &b)` — konwersja HSV → RGB (używana przez rainbow)

### Tryby LED (`led_mode` w config)

| Wartość | Nazwa | Opis |
|---|---|---|
| 0 | Rainbow | Obracający się tęczowy gradient HSV. Jasność z `led_brightness`. Ignoruje kolor RGB. |
| 1 | Static | Stały kolor `(led_r, led_g, led_b)` skalowany przez `led_brightness`. |
| 2 | Breathe | Pulsujący kolor — `(led_r, led_g, led_b)` z narastającą/malejącą jasnością do `led_brightness`. |

### LED Toggle overlay

Każdy z 3 klawiszy może mieć włączony tryb toggle (bit w `led_toggle`):
- Przy naciśnięciu klawisza → flip stanu LED (`led_toggle_state ^= bit`)
- Jeśli LED jest "off" → piksel jest czarny (nadpisanie po kalkulacji trybu)
- Toggle działa niezależnie od wybranego trybu LED

---

## Bootloader

Funkcja `enterBootloader()` w `main.c` przełącza MCU w tryb bootloadera ROM (adres `0x3800`):

```c
void enterBootloader(void) {
    EA = 0;                    // wyłącz wszystkie przerwania
    USB_INT_EN = 0;            // wyłącz przerwania USB
    USB_CTRL   = 0;            // wyłącz kontroler USB
    UDEV_CTRL  = 0;            // zwolnij pull-up D+ → host widzi odłączenie
    delayMicroseconds(50000);  // 4× ~50 ms = ~200 ms — wystarczająco dla hosta
    delayMicroseconds(50000);
    delayMicroseconds(50000);
    delayMicroseconds(50000);
    __asm__("ljmp 0x3800\n");  // skok do bootloadera ROM
    while (1);
}
```

**Kluczowe:** `UDEV_CTRL = 0` jest konieczne, aby host (Windows) wykrył odłączenie urządzenia HID przed ponowną enumeracją w trybie bootloadera. Bez tego host widzi urządzenie jako nadal podłączoną klawiaturę.

Bootloader jest wywoływany zdalnie z aplikacji Windows przez Feature Report ID 4 z komendą `0x42`.

---

## Aplikacja konfiguracyjna Windows (`src/app/main.cpp`)

Aplikacja desktopowa C++17 korzystająca z **JQB_WindowsLib** (`lib_deps` w `platformio.ini`).

### Architektura

- Model Arduino-like: `init()` / `setup()` / `loop()`
- Komunikacja przez **HID Feature Reports** (klasa `HID` z JQB_WindowsLib)
- Auto-connect: `tryConnect()` — automatycznie próbuje połączyć się z klawiaturą przed każdą operacją
- Live preview LED — `loop()` pollinguje zmiany w kontrolkach LED i wysyła Report 5 na bieżąco

### UI — TabControl z 3 zakładkami

| Zakładka | Zawartość |
|---|---|
| **Klawisze** | Combo-capture buttony (kliknij → naciśnij kombinację klawiszy, np. Ctrl+Alt+F6) dla 3 klawiszy + enc button + enc CW/CCW. Modyfikatory przechwytywane automatycznie. |
| **LED** | Select trybu (Tęcza/Statyczny/Oddech), jasności (0–255 co 10), koloru (predefiniowane). |
| **LED Toggle** | 3 checkboxy — włącz/wyłącz toggle per klawisz. |

### Przyciski akcji (pod TabControl)

| Przycisk | Akcja |
|---|---|
| **Odczytaj** | GET Feature Reports 2+3+5 → wypełnij UI |
| **Zapisz** | UI → SET Feature Reports 2+3+5 → EEPROM |
| **Domyślne** | Przywróć domyślne w UI (bez zapisu) |
| **Bootloader** | SET Feature Report 4 z `CMD_BOOTLOADER` → MCU wchodzi w bootloader |

### Key-capture / Combo-capture (przechwytywanie kombinacji klawiszy)

Buttony na zakładce "Klawisze" używają `SetWindowSubclass()` z custom `KeyCaptureProc`:
1. Kliknięcie → przycisk pokazuje "..." i czeka na kombinację klawiszy
2. `WM_KEYDOWN` / `WM_SYSKEYDOWN` → `vkToFwKey()` konwertuje VK na kod firmware, `getAsyncModBits()` odczytuje aktywne modyfikatory jako bitmaskę
3. Rozróżnia lewy/prawy Shift/Ctrl/Alt przez `GetAsyncKeyState()`, `fwKeyToModBit()` usuwa bit samego modyfikatora jeśli to on jest naciskanym klawiszem
4. Wyświetla kombinację w formacie "LCtrl+LAlt+F6" przez `comboToName(mod, code)`
5. Focus loss → anuluje przechwytywanie

Struktura `KeyCapture` przechowuje `uint8_t mod` (bitmask) + `uint8_t code` (key code).

### HID — połączenie z klawiaturą

```cpp
hid.init();
hid.setVidPid(KB_VID, KB_PID);         // 0x1209, 0xC55D
hid.setUsage(VENDOR_USAGE_PAGE, VENDOR_USAGE);  // 0xFF00, 0x01
hid.setFeatureReportSize(CONFIG_REPORT_SIZE);    // 7
hid.findAndOpen();
```

### Konwersje kodów klawiszy

- `vkToFwKey(UINT vk)` — Windows VK → firmware key code (ASCII / `KC_*` z `protocol.h`)
- `fwKeyToName(uint8_t code)` — firmware key code → nazwa do wyświetlenia (np. `0xDA` → "Up")
- `fwKeyToModBit(uint8_t code)` — firmware key code → odpowiadający bit MOD_BIT_* (0 jeśli nie modyfikator)
- `modBitsToPrefix(uint8_t mod)` — bitmask modyfikatorów → string prefix (np. "LCtrl+LAlt+")
- `comboToName(uint8_t mod, uint8_t code)` — pełna nazwa kombinacji (np. "LCtrl+LAlt+F6")
- `getAsyncModBits()` — odczytuje aktualnie naciśnięte modyfikatory przez `GetAsyncKeyState()` jako bitmaskę

---

## Dostępne API (ch55xduino) — firmware

```c
#include <Arduino.h>

// GPIO
void pinMode(uint8_t pin, uint8_t mode);       // INPUT, OUTPUT, INPUT_PULLUP
void digitalWrite(uint8_t pin, uint8_t val);    // HIGH, LOW
uint8_t digitalRead(uint8_t pin);

// Analog
uint16_t analogRead(uint8_t pin);   // piny: 11, 14, 15, 32
void analogWrite(uint8_t pin, uint16_t val); // PWM: 15, 30, 31, 34

// Czas
void delay(uint32_t ms);
void delayMicroseconds(uint16_t us);
uint32_t millis(void);

// EEPROM (DataFlash, 128 B)
void eeprom_write_byte(__data uint8_t addr, __xdata uint8_t val);
uint8_t eeprom_read_byte(__data uint8_t addr);

// USB HID Keyboard (aktywny tryb — wymaga -DUSER_USB_RAM=148)
void USBInit(void);
void Keyboard_press(uint8_t key);
void Keyboard_release(uint8_t key);
void Keyboard_write(uint8_t key);
void Keyboard_releaseAll(void);
// Stałe: KEY_LEFT_SHIFT, KEY_UP_ARROW, KEY_DOWN_ARROW, KEY_RETURN, itd.
```

> **USB Serial CDC (`USBSerial_*`) jest niedostępny** — wyłączony przez `-DUSER_USB_RAM=148`.

---

## Struktura projektu

```
platformio.ini              # Konfiguracja PlatformIO (env:board + env:app)
resources.rc                # Zasoby Windows dla aplikacji (ikona)
src/
  board/
    main.c                  # Firmware: composite HID + encoder + NeoPixel + bootloader
    config.c                # Konfiguracja: EEPROM load/save/reset + pack/unpack
    config.h                # KeyboardConfig struct + API deklaracje
    userUsbHidKeyboard/     # Własna implementacja composite USB HID
      USBconstant.c/h       #   Deskryptory USB (2 interfejsy, Feature Reports)
      USBhandler.c/h        #   Obsługa przerwań USB (EP0 SETUP/IN/OUT, EP1/EP2 IN)
      USBHIDKeyboard.c/h    #   API klawiatury (press/release/write)
  app/
    main.cpp                # Aplikacja konfiguracyjna Windows (C++17, JQB_WindowsLib)
  shared/
    protocol.h              # Wspólny protokół: Report IDs, EEPROM layout, kody klawiszy
platform/ch552/
  platform.json             # Manifest platformy PlatformIO
  platform.py               # Klasa platformy PlatformIO
  sdcc_compat.h             # Stuby słów kluczowych SDCC dla IntelliSense
  setup.ps1                 # Skrypt instalujący toolchain (SDCC + ch55xduino)
  boards/
    ch552g.json             # Definicja płytki CH552G
  builder/
    main.py                 # Skrypt budowania SCons (SDCC) + auto-gen IntelliSense
    frameworks/
      arduino.py            # Integracja z ch55xduino
.vscode/
  c_cpp_properties.json     # Auto-generowany przez builder (nie edytować ręcznie!)
```

---

## Budowanie i wgrywanie

```bash
# Budowanie firmware (automatycznie regeneruje .vscode/c_cpp_properties.json)
pio run -e board

# Wgrywanie firmware (USB bootloader — podłącz CH552G w trybie bootloadera)
pio run -e board -t upload

# Budowanie aplikacji konfiguracyjnej Windows
pio run -e app

# Uruchomienie aplikacji Windows
pio run -e app -t upload

# Pierwsze uruchomienie — instalacja toolchaina (SDCC + ch55xduino + vnproch55x)
powershell -ExecutionPolicy Bypass -File platform\ch552\setup.ps1
```

### Upload — uwagi

- Narzędzie: **vnproch55x** (w `tool-ch55xtools`).
- CH552G musi być w trybie bootloadera (przytrzymaj przycisk boot przy podłączeniu USB, albo wyślij komendę bootloadera z aplikacji).
- Builder zawiera wrapper ignorujący błędy weryfikacji (`"Packet N doesn't match"` — firmware jest wgrany poprawnie, weryfikacja może zawodzić na niektórych wersjach bootloadera).
- Konwersja IHX → BIN odbywa się automatycznie w builderze.
- Alternatywny programator: `ch55xtool` (Python/libusb) — `pip install ch55xtool`.

---

## IntelliSense

Builder automatycznie generuje `.vscode/c_cpp_properties.json` przy każdym `pio run`. Plik `platform/ch552/sdcc_compat.h` jest ustawiony jako `forcedInclude` — mapuje słowa kluczowe SDCC (`__xdata`, `__sfr`, `__at()`, `__sbit`, itd.) na standardowe C, żeby IntelliSense nie zgłaszał błędów.

**Nie edytuj ręcznie** `.vscode/c_cpp_properties.json` — zostanie nadpisany przy następnym buildzie.

---

## Wskazówki dla Copilota

### Firmware (`env:board`)

1. **Zawsze generuj kod w C**, nigdy w C++. Rozszerzenie `.c`.
2. Pamiętaj o ograniczeniach pamięci — 876 B XRAM, 14 KB Flash, 128 B EEPROM.
3. Piny USB (P3.6 = 36, P3.7 = 37) są zarezerwowane — nie konfiguruj ich jako GPIO.
4. Firmware działa jako **composite HID** — `USBSerial_*()` jest **niedostępne**.
5. Do obsługi klawiszy: `Keyboard_press()` / `Keyboard_release()` z `USBHIDKeyboard.h`.
6. Konfiguracja klawiszy jest w `g_config` (EEPROM-backed) — **nie w `#define`**.
7. Przy GET_REPORT: `Ep0Buffer[0] = Report ID`, dane od `Ep0Buffer[1]`, `len = 1 + CONFIG_REPORT_SIZE`.
8. Przy SET_REPORT (EP0_OUT): dane od `Ep0Buffer[1]` (skip Report ID at [0]).
9. Debouncing — polling w `loop()` z `SCAN_DELAY_MS` (5 ms).
10. Enkoder — detekcja zbocza na kanale A, odczyt B dla kierunku.
11. NeoPixel: `set_pixel_for_GRB_LED()` + `neopixel_show_P3_4()`. Bufor w XRAM.
12. **Unikaj `printf` / `sprintf`** — zbyt duże dla 8051.
13. Mnożenie 8-bit: zawsze castuj na `(uint16_t)` — SDCC int = 16-bit signed, łatwo o overflow.
14. Bootloader: wymagane `UDEV_CTRL = 0` żeby host wykrył odłączenie USB przed re-enumeracją.

### Aplikacja Windows (`env:app`)

15. Używa `platform = native` i `JQB_WindowsLib` (GitHub) jako `lib_deps`.
16. Flagi C++17, UNICODE, statyczne linkowanie — dodawane automatycznie przez bibliotekę.
17. Model: `init()` / `setup()` / `loop()` — funkcje globalne, nie w klasie.
18. HID: `hid.init()` → `hid.setVidPid()` → `hid.setUsage()` → `hid.setFeatureReportSize()` → `hid.findAndOpen()`.
19. `tryConnect()` — auto-connect helper wywoływany przed Read/Write/Bootloader.
20. Live preview LED — `loop()` pollinguje zmiany Select/CheckBox i wysyła Report 5.
21. Key-capture — `SetWindowSubclass()` + `WM_KEYDOWN` → `vkToFwKey()` + `getAsyncModBits()` → combo (mod bitmask + key code).
22. **Kod współdzielony:** `src/shared/protocol.h` — includowany przez oba środowiska (C i C++).
