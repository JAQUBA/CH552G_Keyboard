# CH552G Keyboard — Copilot Instructions

## Projekt

Firmware dla 3-klawiszowej klawiatury HID z enkoderem obrotowym i podświetleniem NeoPixel,
opartej na mikrokontrolerze **WCH CH552G** (architektura MCS51 / 8051).  
Projekt zawiera dwa środowiska PlatformIO:
- **board** — firmware CH552G (SDCC, język C)
- **app** — aplikacja konfiguracyjna Windows (MinGW GCC, C++17, biblioteka JQB_WindowsLib)

Budowany za pomocą **PlatformIO** z lokalną platformą (`platform/ch552`) i frameworkiem **ch55xduino**.

## Architektura i ograniczenia

| Parametr | Wartość |
|---|---|
| MCU | WCH CH552G (MCS51 / 8051) |
| Zegar | 24 MHz |
| Flash | 14 336 B (16 KB − 2 KB bootloader) |
| XRAM | 876 B (1 KB − 148 B USB endpoints) |
| RAM wewnętrzny | 256 B |
| Kompilator | **SDCC** (Small Device C Compiler) |
| Język | **Tylko C** — SDCC dla MCS51 **nie obsługuje C++** |
| Model pamięci | `--model-large --int-long-reent` |

### Krytyczne zasady

- **Nie używaj C++** — brak klas, szablonów, `new`, `delete`, przestrzeni nazw, referencji `&`.
- Pliki źródłowe muszą mieć rozszerzenie **`.c`** (nie `.cpp`).
- Zamiast metod obiektowych używaj funkcji C z frameworka, np.:
  - `USBSerial_print()` zamiast `Serial.print()`
  - `USBSerial_println()` zamiast `Serial.println()`
  - `USBSerial_available()`, `USBSerial_read()`, `USBSerial_write()`
- Pamięć jest ekstremalnie ograniczona — unikaj dużych buforów i rekurencji.
- Zmienne globalne domyślnie trafiają do XRAM (`__xdata`). Dla krytycznych czasowo danych użyj `__data` (wewnętrzny RAM, max 256 B).
- **Nie używaj `printf` / `sprintf`** — zbyt duże na 8051. Używaj `USBSerial_print` lub ręcznej konwersji.

## Tryb USB

Firmware działa jako **klawiatura HID** (nie CDC/Serial).

- W `platformio.ini` zdefiniowane jest `-DUSER_USB_RAM=148` — wyłącza domyślny stos USB (CDC) z core ch55xduino i włącza własny.
- Własna implementacja USB HID znajduje się w `src/board/userUsbHidKeyboard/`:
  - `USBconstant.c/h` — deskryptory USB (VID/PID, HID Report Descriptor)
  - `USBhandler.c/h` — obsługa przerwań USB (EP0, EP1)
  - `USBHIDKeyboard.c/h` — API klawiatury: `Keyboard_press()`, `Keyboard_release()`, `Keyboard_write()`
- Inicjalizacja USB: `USBInit()` w `setup()`.
- **USB Serial (`USBSerial_*`) jest niedostępny** gdy `USER_USB_RAM` jest zdefiniowane — cały CDC jest wyłączony.

### Przełączanie trybów USB

| Tryb | `build_flags` | Dostępne API |
|---|---|---|
| HID Keyboard | `-DUSER_USB_RAM=148` | `Keyboard_press()`, `Keyboard_release()`, `Keyboard_write()` |
| CDC Serial | *(brak flagi)* | `USBSerial_print()`, `USBSerial_read()`, itd. |

> Nie można używać HID i CDC jednocześnie w tej implementacji.

## Mapowanie pinów — hardware klawiatury

ch55xduino używa schematu: **`NumerPortu * 10 + NumerPinu`**

### Przypisanie w tym projekcie

| Funkcja | Pin MCU | Numer w kodzie | Konfiguracja |
|---|---|---|---|
| Klawisz 1 (B) | P1.1 | `11` | `INPUT_PULLUP`, aktywny LOW |
| Klawisz 2 (F) | P1.7 | `17` | `INPUT_PULLUP`, aktywny LOW |
| Klawisz 3 (E) | P1.6 | `16` | `INPUT_PULLUP`, aktywny LOW |
| Enkoder kanał A | P3.0 | `30` | `INPUT_PULLUP` |
| Enkoder kanał B | P3.1 | `31` | `INPUT_PULLUP` |
| Przycisk enkodera (J) | P3.3 | `33` | `INPUT_PULLUP`, aktywny LOW |
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

## Konfiguracja klawiszy i enkodera

Mapowanie klawiszy jest definiowane przez `#define` na początku `src/board/main.c` (sekcja **QUICK CONFIG**):

```c
// --- 3 keys with LEDs ---
#define KEY1_PIN   11          // P1.1
#define KEY1_CODE  'b'         // litera do wysłania

// --- Rotary encoder ---
#define ENC_A_PIN  30          // P3.0
#define ENC_B_PIN  31          // P3.1
#define ENC_CW_MOD   KEY_LEFT_SHIFT   // modyfikator: obrót w prawo
#define ENC_CW_KEY   KEY_UP_ARROW     // klawisz: obrót w prawo
#define ENC_CCW_MOD  KEY_LEFT_SHIFT   // modyfikator: obrót w lewo
#define ENC_CCW_KEY  KEY_DOWN_ARROW   // klawisz: obrót w lewo

// --- Encoder button ---
#define ENC_BTN_PIN  33
#define ENC_BTN_CODE KEY_RETURN

// --- NeoPixels ---
#define NUM_LEDS   3
#define LED_BRIGHTNESS 40      // 0-255
```

## NeoPixel (WS2812)

- 3 diody WS2812 na pinie P3.4, format **GRB**.
- Biblioteka: `WS2812` z ch55xduino (włączona przez `build_src_filter` w `platformio.ini`).
- Funkcje:
  - `set_pixel_for_GRB_LED(buffer, index, r, g, b)` — ustaw kolor piksela
  - `neopixel_show_P3_4(buffer, length)` — wyślij dane do LED (dedykowana do pinu P3.4)
- Bufor: `__xdata uint8_t led_data[NUM_LEDS * 3]` (9 bajtów XRAM).
- Domyślny efekt: tęcza (HSV → RGB, przesunięcie hue co klatkę).

## Dostępne API (ch55xduino)

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

// USB (wybierz jeden tryb — patrz sekcja "Tryb USB")
// HID Keyboard (wymaga -DUSER_USB_RAM=148 + pliki w src/userUsbHidKeyboard/)
void USBInit(void);
void Keyboard_press(uint8_t key);
void Keyboard_release(uint8_t key);
void Keyboard_write(uint8_t key);
void Keyboard_releaseAll(void);
// Stałe klawiszy: KEY_LEFT_SHIFT, KEY_UP_ARROW, KEY_DOWN_ARROW, KEY_RETURN, itd.

// USB Serial CDC (domyślny, BEZ -DUSER_USB_RAM)
void USBSerial_print(char *str);
void USBSerial_println(char *str);
uint8_t USBSerial_available(void);
char USBSerial_read(void);
void USBSerial_write(uint8_t c);
```

## Struktura projektu

```
platformio.ini              # Konfiguracja PlatformIO (env:board + env:app)
resources.rc                # Zasoby Windows dla aplikacji (ikona)
src/
  board/
    main.c                  # Firmware: HID keyboard + encoder + NeoPixel
    config.c/h              # Konfiguracja klawiatury (odczyt/zapis z Flash)
    userUsbHidKeyboard/     # Własna implementacja USB HID
      USBconstant.c/h       #   Deskryptory USB
      USBhandler.c/h        #   Obsługa przerwań USB
      USBHIDKeyboard.c/h    #   API klawiatury (press/release/write)
  app/
    main.cpp                # Aplikacja konfiguracyjna Windows (C++17)
  shared/
    protocol.h              # Wspólny protokół komunikacji board ↔ app
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
- CH552G musi być w trybie bootloadera (przytrzymaj przycisk boot przy podłączeniu USB).
- Builder zawiera wrapper ignorujący błędy weryfikacji (`"Packet N doesn't match"` — firmware jest wgrany poprawnie, weryfikacja może zawodzić na niektórych wersjach bootloadera).
- Konwersja IHX → BIN odbywa się automatycznie w builderze.
- Alternatywny programator: `ch55xtool` (Python/libusb) — `pip install ch55xtool`.

## IntelliSense

Builder automatycznie generuje `.vscode/c_cpp_properties.json` przy każdym `pio run`. Plik `platform/ch552/sdcc_compat.h` jest ustawiony jako `forcedInclude` — mapuje słowa kluczowe SDCC (`__xdata`, `__sfr`, `__at()`, `__sbit`, itd.) na standardowe C, żeby IntelliSense nie zgłaszał błędów.

**Nie edytuj ręcznie** `.vscode/c_cpp_properties.json` — zostanie nadpisany przy następnym buildzie.

## Wskazówki dla Copilota

1. **Zawsze generuj kod w C**, nigdy w C++.
2. Pamiętaj o ograniczeniach pamięci — 876 B XRAM, 14 KB Flash.
3. Piny USB (P3.6 = 36, P3.7 = 37) są zarezerwowane — nie konfiguruj ich jako GPIO.
4. Aktualnie firmware działa jako **HID Keyboard** — `USBSerial_*()` jest **niedostępne**.
5. Do obsługi klawiszy używaj `Keyboard_press()` / `Keyboard_release()` z `USBHIDKeyboard.h`.
6. Debouncing przycisków — realizowany przez polling w pętli z `SCAN_DELAY_MS` (5 ms).
7. Enkoder obsługiwany przez polling (detekcja zbocza na kanale A, odczyt B dla kierunku).
8. NeoPixel: użyj `set_pixel_for_GRB_LED()` + `neopixel_show_P3_4()`. Bufor w XRAM.
9. Konfiguracja klawiszy — sekcja `QUICK CONFIG` na początku `src/board/main.c` (define'y).
10. Unikaj `printf` / `sprintf` — zbyt duże dla 8051.
11. **Aplikacja Windows (env:app):** Używa `platform = native` i biblioteki `JQB_WindowsLib` jako `lib_deps`. Flagi C++17, UNICODE i linkowanie dodawane automatycznie przez bibliotekę.
12. **Kod współdzielony:** `src/shared/protocol.h` definiuje wspólny protokół między firmware a aplikacją.
