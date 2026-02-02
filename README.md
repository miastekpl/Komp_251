# Trassar-Painter v7.0.0

**Profesjonalny komputer malowarki drogowej na platformie ESP32-S3**

SD-SPEED EDITION - Obsługa karty SD, kontrola prędkości malowania, jeden przycisk START/PAUZA, poprawiony E-STOP NC.

---

## Funkcje

- 15 wzorców malowania (P-1a do P-7d) zgodnych z polską normą
- 6 kanałów pistoletów (12cm + 24cm)
- Enkoder pomiarowy (dystans + prędkość)
- **Kontrola prędkości** - malowanie tylko powyżej 3 km/h (bezpieczeństwo)
- **Cyklowanie wzorców przerywanych** - automatyczne linia/przerwa
- **Karta SD** - zapis raportów na kartę (fallback na LittleFS)
- Kalibracja automatyczna (jedź 10m)
- Panel WWW (WiFi AP: 192.168.4.1) z Basic Auth
- Raporty pracy z obliczaniem powierzchni m2 + eksport CSV + usuwanie
- RTC DS1307 + synchronizacja NTP (natywne configTime)
- System bezpieczeństwa (E-STOP NC, Watchdog, Heartbeat, Deadman, Self-test)
- Aktualizacja OTA (przez WiFi)
- Wyświetlacz TFT ILI9341 2.8"
- Tryb serwisowy (test pistoletów)

## Struktura projektu

```
Komp_251/
├── src/                    # Kod źródłowy (modularny)
│   ├── main.cpp            # Setup + Loop + ISR (plik główny)
│   ├── pins.h              # GPIO - JEDYNE źródło prawdy o pinach
│   ├── pins.cpp            # Definicje extern const (RELAY_PINS[])
│   ├── config.h            # Konfiguracja systemu
│   ├── types.h             # Struktury i enumy
│   ├── patterns.h/.cpp     # Wzorce malowania (15)
│   ├── state.h/.cpp        # Zmienne globalne
│   ├── safety.h/.cpp       # System bezpieczeństwa
│   ├── encoder.h/.cpp      # Enkoder + kalibracja
│   ├── reports.h/.cpp      # Raporty + CSV + SD
│   ├── rtc_ntp.h/.cpp      # RTC + NTP (configTime)
│   ├── buzzer_leds.h/.cpp  # Buzzer + LEDy
│   ├── display.h/.cpp      # TFT ILI9341
│   ├── sd_card.h/.cpp      # Karta SD (współdzielony SPI)
│   └── web_panel.h/.cpp    # Panel WWW + REST API
├── docs/                   # Dokumentacja
│   ├── INSTRUKCJA_OBSLUGI.md
│   ├── SCHEMAT_POLACZEN.md
│   ├── CHANGELOG.md
│   └── AUDYT_KODU_v6.md
├── platformio.ini          # Konfiguracja PlatformIO
└── README.md
```

## Szybki start

### Wymagania
- PlatformIO (VS Code + rozszerzenie PlatformIO IDE)
- ESP32-S3 DevKitC-1 (16MB Flash, 8MB PSRAM)
- Kabel USB-C

### Kompilacja i upload
```bash
# Kompilacja
pio run

# Upload przez USB
pio run -t upload

# Monitor serial
pio device monitor
```

### Połączenie z panelem WWW
1. WiFi: **Trassar-Painter** (hasło: 12345678)
2. Przeglądarka: **http://192.168.4.1**

## Zmiany w v7.0.0 (względem v6.0.0)

| Zmiana | Opis |
|--------|------|
| **Karta SD** | Raporty na kartę SD, fallback LittleFS |
| **Prędkość min. 3 km/h** | Pistolety OFF poniżej progu |
| **Jeden przycisk START/PAUZA** | GPIO 40 (zamiast osobnych START+PAUSE) |
| **RELAY_4 → GPIO 19** | Z GPIO 45 (strapping pin) na GPIO 19 |
| **E-STOP NC poprawiony** | ISR CHANGE + digitalRead (nie FALLING) |
| **Cyklowanie wzorców** | Automatyczne linia/przerwa wg dystansu |
| **Basic Auth** | Reset E-STOP wymaga hasła |
| **Usuwanie raportów** | DELETE w panelu WWW |
| **NTPClient usunięty** | Natywne configTime() ESP32 |

## Dokumentacja

- [Instrukcja obsługi](docs/INSTRUKCJA_OBSLUGI.md) - pełna instrukcja operatora
- [Schemat połączeń](docs/SCHEMAT_POLACZEN.md) - rozpiska GPIO i schematy elektryczne
- [Changelog](docs/CHANGELOG.md) - historia zmian
- [Audyt kodu v6.0.0](docs/AUDYT_KODU_v6.md) - raport z audytu poprzedniej wersji

## Licencja

Projekt autorstwa Trassar251. Wszelkie prawa zastrzeżone.

---

*Trassar-Painter v7.0.0 SD-SPEED EDITION | ESP32-S3 | 2026-02-02*
