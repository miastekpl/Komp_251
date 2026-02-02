# CHANGELOG - Trassar-Painter

Wszystkie istotne zmiany w projekcie.

---

## [6.0.0] - 2026-02-02 - MODULAR PRODUCTION EDITION

### MAJOR REFACTORING
- **Kompletna modularyzacja kodu** - rozbicie monolitu 3398 linii na 15 plików modułowych
- Nowa struktura katalogów: `src/` (kod), `docs/` (dokumentacja), `include/`

### NOWE PLIKI
| Plik | Opis |
|------|------|
| `src/pins.h` | JEDYNE źródło prawdy o pinach GPIO |
| `src/config.h` | Konfiguracja systemu (timing, WiFi, limity) |
| `src/types.h` | Struktury, enumy (SystemMode, ErrorType, PatternInfo, WorkReport) |
| `src/patterns.h/.cpp` | 15 wzorców malowania |
| `src/state.h/.cpp` | Zmienne globalne |
| `src/safety.h/.cpp` | System bezpieczeństwa |
| `src/encoder.h/.cpp` | Enkoder pomiarowy + kalibracja |
| `src/reports.h/.cpp` | Raporty pracy + CSV export |
| `src/rtc_ntp.h/.cpp` | RTC DS1307 + NTP |
| `src/buzzer_leds.h/.cpp` | Buzzer + LEDy statusu |
| `src/display.h/.cpp` | Wyświetlacz TFT ILI9341 |
| `src/web_panel.h/.cpp` | Panel WWW + REST API |
| `src/main.cpp` | setup() + loop() + ISR (czysty plik główny) |

### NAPRAWIONE BUGI KRYTYCZNE
- **[CRITICAL] E-STOP GPIO** - `activateEmergencyStop()` używała `RELAY_1 + i` (piny 21-26),
  teraz używa tablicy `RELAY_PINS[]` (piny 21,47,48,45,38,39). W v5.3.0 awaryjne wyłączenie
  NIE wyłączało 4 z 6 pistoletów!
- **[CRITICAL] checkHeartbeat()** - to samo naprawienie z RELAY_PINS[]
- **[CRITICAL] CSV Export** - zastąpienie hardkodowanego placeholder prawdziwym parserem
- **[CRITICAL] getReportsListJSON()** - dodanie pola "count" wymaganego przez panel WWW
- **[BUG] Duplikacja TFT_UPDATE_INTERVAL** - usunięta podwójna deklaracja
- **[BUG] Duplikacja GPIO** - config.h miał inne piny niż main.cpp; teraz pins.h jest jedynym źródłem

### NAPRAWIONE BUGI ISTOTNE
- **Walidacja API** - wszystkie endpointy sprawdzają zakresy (pattern index 0-14, report id)
- **String.reserve()** - minimalizacja fragmentacji pamięci przy budowaniu JSON
- **Praca przez północ** - stopReport() obsługuje sesje przechodzące przez 00:00
- **CSV Content-Disposition** - nagłówek pobierania pliku dla eksportu CSV
- **Spójne wersjonowanie** - wszędzie v6.0.0 (był mix v5.0/v5.2/v5.3)

### USUNIĘTE
- Stary monolityczny `main.cpp` (3398 linii) - zastąpiony strukturą modułową
- Stary `config.h` z martwymi definicjami GPIO (15 przycisków wzorców nigdy nieużywanych)
- Komentarz po rosyjsku ("КОНСТАНТЫ KONFIGURACYJNE")
- Martwa konfiguracja RTOS (priorytety, stacki, przypisania rdzeni bez tasków)

---

## [5.3.0] - 2026-01-19 - PRODUCTION SAFETY + Raporty + RTC + NTP + OTA

### Dodane
- Production-grade Safety: E-STOP, Watchdog, Heartbeat, Deadman Switch
- Self-test diagnostyczny przy starcie
- Error logging (circular buffer 50 błędów + plik /errors.log)
- Encoder Health Check (stall + disconnection detection)
- Speed Validation (0-25 km/h)
- Calibration Drift Detection (>20%)
- Status LEDs (Green/Yellow/Red) + Buzzer alerts
- CSV Export raportów (placeholder)
- Safety REST API (/api/safety/status, /api/safety/errors, /api/safety/reset-estop)

### Znane problemy (naprawione w v6.0.0)
- Bug E-STOP GPIO (RELAY_1 + i zamiast tablicy pinów)
- CSV export zwracał hardkodowane dane
- Panel WWW oczekiwał "count" w JSON raportów
- Brak walidacji zakresów w API

---

## [5.2.0] - 2026-01-15 - Raporty pracy + RTC + NTP + OTA

### Dodane
- Raporty pracy z automatycznym raportowaniem powierzchni m²
- RTC DS1307 (zegar czasu rzeczywistego)
- NTP synchronizacja czasu (strefa polska UTC+1/+2 z DST)
- OTA (Over-The-Air Updates)
- Zakładka Raporty w panelu WWW
- API raportów (/api/reports, /api/report, /api/report/current)

---

## [5.0.0] - 2026-01-10 - FINAL (TFT + Joystick)

### Dodane
- Panel WWW z dark theme (4 karty: Główny/Pomiar/Kalibracja/Raporty)
- 15 wzorców malowania zgodnych z normą
- Tryb serwisowy (test pistoletów)
- Kalibracja automatyczna (10m)
- Selektor P3 (fizyczny + wirtualny)
- Start od przerwy
- Dual WiFi (AP + STA)
- Wyświetlacz TFT ILI9341
- Joystick analogowy (nawigacja menu)
