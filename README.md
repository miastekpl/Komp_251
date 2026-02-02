# Trassar-Painter v6.0.0

**Profesjonalny komputer malowarki drogowej na platformie ESP32-S3**

MODULAR PRODUCTION EDITION - Pełna modularyzacja kodu, naprawione krytyczne bugi, dokumentacja produkcyjna.

---

## Funkcje

- 15 wzorców malowania (P-1a do P-7d) zgodnych z polską normą
- 6 kanałów pistoletów (12cm + 24cm)
- Enkoder pomiarowy (dystans + prędkość)
- Kalibracja automatyczna (jedź 10m)
- Panel WWW (WiFi AP: 192.168.4.1)
- Raporty pracy z obliczaniem powierzchni m2 + eksport CSV
- RTC DS1307 + synchronizacja NTP
- System bezpieczeństwa (E-STOP, Watchdog, Heartbeat, Deadman, Self-test)
- Aktualizacja OTA (przez WiFi)
- Wyświetlacz TFT ILI9341 (opcjonalnie)
- Tryb serwisowy (test pistoletów)

## Struktura projektu

```
Komp_251/
├── src/                    # Kod źródłowy (modularny)
│   ├── main.cpp            # Setup + Loop + ISR (plik główny)
│   ├── pins.h              # GPIO - JEDYNE źródło prawdy o pinach
│   ├── config.h            # Konfiguracja systemu
│   ├── types.h             # Struktury i enumy
│   ├── patterns.h/.cpp     # Wzorce malowania (15)
│   ├── state.h/.cpp        # Zmienne globalne
│   ├── safety.h/.cpp       # System bezpieczeństwa
│   ├── encoder.h/.cpp      # Enkoder + kalibracja
│   ├── reports.h/.cpp      # Raporty + CSV
│   ├── rtc_ntp.h/.cpp      # RTC + NTP
│   ├── buzzer_leds.h/.cpp  # Buzzer + LEDy
│   ├── display.h/.cpp      # TFT ILI9341
│   └── web_panel.h/.cpp    # Panel WWW + REST API
├── docs/                   # Dokumentacja
│   ├── INSTRUKCJA_OBSLUGI.md
│   ├── SCHEMAT_POLACZEN.md
│   └── CHANGELOG.md
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

## Dokumentacja

- [Instrukcja obsługi](docs/INSTRUKCJA_OBSLUGI.md) - pełna instrukcja operatora
- [Schemat połączeń](docs/SCHEMAT_POLACZEN.md) - rozpiska GPIO i schematy elektryczne
- [Changelog](docs/CHANGELOG.md) - historia zmian

## Naprawione bugi w v6.0.0

| Bug | Opis | Krytyczność |
|-----|------|-------------|
| E-STOP GPIO | `RELAY_1+i` dawał złe piny; teraz `RELAY_PINS[]` | KRYTYCZNY |
| CSV Export | Hardkodowany placeholder zamiast parsera | KRYTYCZNY |
| JSON count | Brak pola "count" w API raportów | KRYTYCZNY |
| Walidacja API | Brak sprawdzania zakresów (pattern, report id) | ISTOTNY |
| Duplikacja GPIO | Dwa zestawy pinów w config.h vs main.cpp | ISTOTNY |
| Praca przez północ | stopReport() dawał workDuration=0 | ISTOTNY |

## Licencja

Projekt autorstwa Trassar251. Wszelkie prawa zastrzeżone.

---

*Trassar-Painter v6.0.0 | ESP32-S3 | 2026-02-02*
