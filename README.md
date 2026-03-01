# CH552G Keyboard

Firmware i aplikacja konfiguracyjna dla **3-klawiszowej klawiatury USB HID** z enkoderem obrotowym i podświetleniem NeoPixel, opartej na mikrokontrolerze **WCH CH552G**.

## Funkcje

- **3 programowalne klawisze** — dowolny klawisz lub kombinacja (np. Ctrl+Alt+F6)
- **Enkoder obrotowy** — programowalny przycisk + obrót CW/CCW z modyfikatorami
- **Podświetlenie NeoPixel** (3× WS2812) — tryby: tęcza, kolor statyczny, oddychanie
- **LED toggle per-key** — każdy klawisz może przełączać swoją diodę
- **Konfiguracja przez USB** — aplikacja Windows komunikuje się przez HID Feature Reports
- **EEPROM** — ustawienia zapisywane w DataFlash, przeżywają restart

## Hardware

| Parametr | Wartość |
|---|---|
| MCU | WCH CH552G (MCS51 / 8051, 24 MHz) |
| Flash | 14 KB (16 KB − 2 KB bootloader) |
| XRAM | 876 B |
| DataFlash (EEPROM) | 128 B |
| USB | Composite HID (Keyboard + Vendor) |
| VID/PID | `0x1209` / `0xC55D` |

### Pinout

| Funkcja | Pin |
|---|---|
| Klawisz 1 | P1.1 |
| Klawisz 2 | P1.7 |
| Klawisz 3 | P1.6 |
| Enkoder A | P3.0 |
| Enkoder B | P3.1 |
| Przycisk enkodera | P3.3 |
| NeoPixel (WS2812) | P3.4 |

## Struktura projektu

```
src/
  board/           Firmware CH552G (C, SDCC)
    main.c           Logika klawiatury, enkoder, NeoPixel
    config.c/h       Konfiguracja EEPROM
    userUsbHidKeyboard/  Composite USB HID stack
  app/
    main.cpp         Aplikacja konfiguracyjna Windows (C++17)
  shared/
    protocol.h       Współdzielony protokół (firmware + app)
platform/ch552/    Lokalna platforma PlatformIO
```

Projekt używa **PlatformIO** z dwoma środowiskami:
- **`board`** — firmware (SDCC, C)
- **`app`** — aplikacja Windows (MinGW GCC, C++17, [JQB_WindowsLib](https://github.com/JAQUBA/JQB_WindowsLib))

## Wymagania

- [PlatformIO](https://platformio.org/) (CLI lub VS Code extension)
- Toolchain instalowany automatycznie przez `setup.ps1`

## Pierwsze uruchomienie

```powershell
# Instalacja toolchaina (SDCC + ch55xduino + vnproch55x)
powershell -ExecutionPolicy Bypass -File platform\ch552\setup.ps1
```

## Budowanie

```bash
# Firmware
pio run -e board

# Aplikacja Windows
pio run -e app
```

## Wgrywanie firmware

```bash
pio run -e board -t upload
```

CH552G musi być w trybie bootloadera — przytrzymaj przycisk boot przy podłączaniu USB lub wyślij komendę bootloadera z aplikacji.

## Uruchomienie aplikacji

```bash
pio run -e app -t upload
```

## Aplikacja konfiguracyjna

Aplikacja Windows z interfejsem zakładkowym:

| Zakładka | Opis |
|---|---|
| **Klawisze** | Przypisz kombinacje klawiszy (kliknij → naciśnij combo, np. Ctrl+F6) |
| **LED** | Tryb podświetlenia, jasność, kolor |
| **LED Toggle** | Włącz/wyłącz toggle LED per klawisz |

Przyciski: **Odczytaj** / **Zapisz** / **Domyślne** / **Bootloader**

Komunikacja z klawiaturą odbywa się przez HID Feature Reports (VID `0x1209`, PID `0xC55D`, Usage Page `0xFF00`).

## Protokół USB

Konfiguracja przez 3 Feature Reports na EP0:

| Report ID | Funkcja |
|---|---|
| 2 | Mapowanie klawiszy (modifier bitmask + key code) |
| 3 | Mapowanie enkodera (modifier bitmask + key code) |
| 4 | Komenda (0x42 = bootloader) |
| 5 | Konfiguracja LED (tryb, jasność, kolor, toggle) |

Każdy klawisz i funkcja enkodera obsługuje **kombinacje z modyfikatorami** (Ctrl, Shift, Alt, Win — lewy/prawy).

## Licencja

MIT
