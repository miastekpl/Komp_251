# AUDYT KODU - Trassar-Painter v6.0.0

## Raport kompleksowej analizy kodu

**Data audytu:** 2026-02-02
**Audytor:** Inżynier IT (200 lat doświadczenia + zespół wsparcia)
**Zakres:** Pełna analiza 15 plików źródłowych, pinów GPIO, logiki biznesowej, bezpieczeństwa

---

## PODSUMOWANIE KRYTYCZNOŚCI

| Poziom | Ilość | Opis |
|--------|-------|------|
| **KRYTYCZNY** | 2 | Brakująca logika cyklowania wzorców, E-STOP edge |
| **POWAŻNY** | 4 | GPIO 45 strapping pin, static w headerze, brak autentykacji WWW, SD card |
| **UMIARKOWANY** | 5 | Fragmentacja pamięci, hardcoded WiFi, buzzer logic, deadman error type, brak usuwania raportów |
| **NISKI** | 3 | Brak CORS, NTP podwójna konfiguracja, TFT wyłączony |

---

## 1. PROBLEMY KRYTYCZNE

### 1.1 BRAK LOGIKI CYKLOWANIA WZORCÓW PRZERYWANYCH (KRYTYCZNY)

**Plik:** `src/main.cpp:128-165` (funkcja `updateGuns()`)

**Problem:** Funkcja `updateGuns()` ustawia pistolety według tablicy `patterns[].guns[]`, ale **NIE implementuje cyklowania linia/przerwa** dla wzorców przerywanych. Dla wzorca P-1a (4m linia, 8m przerwa) pistolety powinny:
- Być ON przez 4 metry
- Być OFF przez 8 metrów
- Powtarzać cykl

Aktualnie pistolety są ZAWSZE ON gdy `MODE_WORKING`, niezależnie od `lineLength` i `gapLength` wzorca. Jedyny wyjątek to `startFromGap` (jednorazowa przerwa na starcie).

**Skutek:** Wszystkie wzorce przerywane (P-1a, P-1b, P-1c, P-1d, P-1e, P-3a, P-3b, P-6, P-7a, P-7c) malują jak ciągłe! To 10 z 15 wzorców.

**Remediacja:** Dodać zmienną śledzącą pozycję w cyklu wzorca (`patternCycleDistance`). W `updateDistanceAndSpeed()` (encoder.cpp) lub `updateGuns()` sprawdzać:
```
float cycleLength = lineLength + gapLength;
float posInCycle = fmod(distanceTraveled, cycleLength);
bool inLine = (posInCycle < lineLength);
// jeśli inLine → pistolety ON, else → OFF
```

### 1.2 E-STOP ISR - POTENCJALNIE ZŁY EDGE (KRYTYCZNY)

**Plik:** `src/main.cpp:401`

**Problem:** ISR E-STOP używa `FALLING` edge:
```cpp
attachInterrupt(digitalPinToInterrupt(BTN_EMERGENCY_STOP), emergencyStopISR, FALLING);
```

Dokumentacja w `docs/SCHEMAT_POLACZEN.md:73-76` mówi:
- Normalny stan (E-STOP zwolniony): GPIO = **LOW** (NC = zamknięty = GND)
- Wciśnięty: GPIO = **HIGH** (obwód otwarty = PULLUP)

**FALLING** = przejście HIGH→LOW = moment **zwolnienia** E-STOP, NIE wciśnięcia!

Dla przycisku NC wciśnięcie = przejście LOW→HIGH = **RISING**.

**Uwaga:** To zależy od fizycznego okablowania. Jeśli przycisk jest NO (Normally Open) a nie NC, to FALLING jest poprawny. Dokumentacja mówi NC, ale kod zakłada zachowanie typu NO.

**Remediacja:** Zweryfikować fizyczne okablowanie:
- Jeśli NC → zmienić na `RISING`
- Jeśli NO → poprawić dokumentację
- Najlepiej: użyć `CHANGE` i sprawdzać stan w ISR:
```cpp
void IRAM_ATTR emergencyStopISR() {
    if (digitalRead(BTN_EMERGENCY_STOP) == HIGH) { // Wciśnięty (NC otworzony)
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_emergencyStop = true;
        portEXIT_CRITICAL_ISR(&isr_mux);
    }
}
```

---

## 2. PROBLEMY POWAŻNE

### 2.1 GPIO 45 - STRAPPING PIN ESP32-S3 (POWAŻNY)

**Plik:** `src/pins.h:26`

**Problem:** GPIO 45 jest używany jako `RELAY_4` (pistolet P4, 24cm). Na ESP32-S3 GPIO 45 jest **strapping pinem** kontrolującym napięcie VDD_SPI:
- GPIO 45 = LOW przy boot → VDD_SPI = 3.3V (domyślne)
- GPIO 45 = HIGH przy boot → VDD_SPI = 1.8V

Jeśli przekaźnik lub obwód zewnętrzny pociąga GPIO 45 do HIGH podczas startu, ESP32-S3 może przełączyć flash/PSRAM na 1.8V i **nie uruchomić się**.

**Remediacja:**
1. Dodać rezystor pull-down 10kΩ na GPIO 45 (zapewni LOW przy boot)
2. Upewnić się, że tranzystor sterujący przekaźnikiem nie pociąga bazy/gate HIGH przy starcie
3. Alternatywnie: przenieść RELAY_4 na inny GPIO (np. GPIO 1, 2, 3 jeśli wolne)

### 2.2 GPIO 46 - STRAPPING PIN ESP32-S3 (UWAGA)

**Plik:** `src/pins.h:46`

**Problem:** GPIO 46 (`BUZZER_PIN`) jest również strapping pinem na ESP32-S3. Ma wewnętrzny pull-down. Kontroluje tryb logowania boot ROM.

**Remediacja:** GPIO 46 jako buzzer jest akceptowalny (LOW przy boot = cisza), ale dodać notatkę w dokumentacji. Nie podłączać pull-up na tym pinie.

### 2.3 `static const` W PLIKU NAGŁÓWKOWYM (POWAŻNY - KOMPILACJA)

**Plik:** `src/pins.h:31-32`

**Problem:**
```cpp
static const int RELAY_PINS[6] = {...};
static const int RELAY_COUNT = 6;
```

`static const` w pliku `.h` tworzy **osobną kopię** w każdym pliku `.cpp` który go includuje. Dla 8+ plików cpp to 8 kopii tablicy. Poza marnowaniem pamięci, może to powodować problemy z adresami.

**Remediacja:** Zmienić na:
```cpp
// pins.h
extern const int RELAY_PINS[6];
extern const int RELAY_COUNT;

// pins.cpp (nowy plik)
#include "pins.h"
const int RELAY_PINS[6] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4, RELAY_5, RELAY_6};
const int RELAY_COUNT = 6;
```

Alternatywnie użyć `constexpr` (C++17) lub `inline constexpr` (C++17).

### 2.4 BRAK AUTENTYKACJI PANELU WWW (POWAŻNY)

**Plik:** `src/web_panel.cpp`

**Problem:** Panel WWW nie ma żadnej autentykacji. Każdy kto połączy się z WiFi AP może:
- Sterować pistoletami
- Zmieniać wzorce
- Resetować E-STOP (endpoint `/api/safety/reset-estop`)
- Uruchamiać OTA

**Remediacja:**
1. Dodać Basic Auth do krytycznych endpointów (reset-estop, OTA, cmd)
2. Minimalne rozwiązanie: `server.authenticate("admin", "haslo")` przed wykonaniem akcji
3. Hasło WiFi AP (12345678) jest bardzo słabe - zmienić na silniejsze

### 2.5 SD CARD - BRAKUJĄCY PIN CS (POWAŻNY)

**Kontekst:** Użytkownik ma czytnik kart SD zintegrowany z wyświetlaczem TFT ILI9341.

**Problem:** Brak definicji pinu CS dla karty SD. SD card reader dzieli magistralę SPI z TFT (MOSI=11, MISO=13, SCK=12) ale potrzebuje **osobnego pinu CS**.

**Remediacja:**
1. Zdefiniować pin SD_CS w `pins.h` (np. GPIO 1, 2 lub 3)
2. Dodać moduł `sd_card.h/.cpp` do obsługi karty SD
3. Użyć `SD.begin(SD_CS_PIN)` po zainicjalizowaniu SPI
4. Przenieść raporty i logi z LittleFS na SD card (większa pojemność)
5. **WAŻNE:** Przy współdzieleniu SPI z TFT, zawsze przed operacją SD deaktywować CS TFT i vice versa

---

## 3. PROBLEMY UMIARKOWANE

### 3.1 FRAGMENTACJA PAMIĘCI - String CONCATENATION (UMIARKOWANY)

**Pliki:** `src/web_panel.cpp:42-93`, `src/reports.cpp:178-201,238-359`

**Problem:** Budowanie JSON i CSV przez wielokrotne `json += String(...)`. Mimo `reserve()`, każde `+=` może realokować pamięć. Na ESP32 z ograniczonym heapem prowadzi do fragmentacji.

**Remediacja:**
1. Użyć `snprintf()` z buforem stałego rozmiaru dla krytycznych JSON
2. Rozważyć ArduinoJson library (efektywna serializacja)
3. Dla CSV export - pisać bezpośrednio do response streama zamiast budować cały String

### 3.2 HARDCODED WIFI CREDENTIALS (UMIARKOWANY)

**Plik:** `src/config.h:28-29`

**Problem:** SSID i hasło WiFi STA hardcoded ("Miastek_WiFi" / "12345678").

**Remediacja:**
1. Przenieść do NVS (Preferences) z domyślnymi wartościami
2. Dodać endpoint w panelu WWW do zmiany credentials
3. Portal captive na pierwszym uruchomieniu

### 3.3 BUZZER UPDATE LOGIC (UMIARKOWANY)

**Plik:** `src/buzzer_leds.cpp:25-45`

**Problem:** Logika `buzzerUpdate()` dekrementuje `buzzerBeepCount` w każdym cyklu OFF, ale nie śledzi poprawnie numerów cykli. Może powodować nierówne beepy lub przedwczesne zakończenie.

**Remediacja:** Przepisać z czystym licznikiem cykli:
```cpp
int currentBeep = elapsed / (BUZZER_CYCLE_MS * 2); // numer beep-a
if (currentBeep >= buzzerBeepCount) { buzzerActive = false; return; }
bool on = (elapsed % (BUZZER_CYCLE_MS * 2)) < BUZZER_CYCLE_MS;
digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
```

### 3.4 DEADMAN SWITCH LOGUJE ZŁY TYP BŁĘDU (UMIARKOWANY)

**Plik:** `src/safety.cpp:175`

**Problem:**
```cpp
logError(ERR_HEARTBEAT_TIMEOUT, "Deadman switch timeout...");
```
Używa `ERR_HEARTBEAT_TIMEOUT` zamiast dedykowanego typu. Brak `ERR_DEADMAN_TIMEOUT` w enumie.

**Remediacja:** Dodać `ERR_DEADMAN_TIMEOUT` do `types.h` i użyć w `checkDeadman()`.

### 3.5 BRAK USUWANIA RAPORTÓW (UMIARKOWANY)

**Problem:** Brak endpointu API do usuwania raportów. LittleFS ma ograniczoną pojemność. Po wypełnieniu użytkownik nie może zwolnić miejsca bez formatu.

**Remediacja:** Dodać:
1. `DELETE /api/report?id=X` - usunięcie pojedynczego raportu
2. `DELETE /api/reports/all` - usunięcie wszystkich
3. Automatyczne rotowanie (usuwanie najstarszych gdy brak miejsca)

---

## 4. PROBLEMY NISKIE

### 4.1 NTP PODWÓJNA KONFIGURACJA (NISKI)

**Plik:** `src/rtc_ntp.cpp:54-56`

**Problem:** `syncNTP()` używa zarówno `configTime()` jak i `NTPClient`. To dwa różne mechanizmy synchronizacji czasu. `configTime()` to natywna funkcja ESP32 IDF, `NTPClient` to biblioteka Arduino.

**Remediacja:** Wybrać jedno podejście. Dla ESP32 zalecane jest `configTime()` + `getLocalTime()`.

### 4.2 BRAK CORS HEADERS (NISKI)

**Plik:** `src/web_panel.cpp`

**Problem:** Brak nagłówków CORS. Nie wpływa na panel wbudowany, ale uniemożliwia dostęp z zewnętrznych aplikacji.

### 4.3 TFT WYŁĄCZONY ALE INICJALIZOWANY (NISKI)

**Plik:** `src/main.cpp:64`, `src/display.cpp:28-49`

**Problem:** Obiekt `TFT_eSPI tft` jest tworzony i `tftInit()` jest wywoływane, ale kod jest zakomentowany. TFT_eSPI zajmuje pamięć RAM nawet bez aktywnego wyświetlacza.

**Remediacja:** Użyć `#ifdef USE_TFT` do warunkowej kompilacji.

---

## 5. ANALIZA PINÓW GPIO

### 5.1 Mapa wykorzystania GPIO ESP32-S3

| GPIO | Funkcja | Status | Uwagi |
|------|---------|--------|-------|
| 0 | - | **WOLNY** | Strapping pin (boot mode) |
| 1 | - | **WOLNY** | Kandydat na SD_CS |
| 2 | - | **WOLNY** | Kandydat na SD_CS |
| 3 | - | **WOLNY** | Strapping pin |
| 4 | JOY_VRX | OK | ADC1_CH3 |
| 5 | JOY_VRY | OK | ADC1_CH4 |
| 6 | JOY_SW | OK | |
| 7 | RTC_SDA | OK | I2C Data |
| 8 | ENC_CLK | OK | ISR RISING |
| 9 | ENC_DT | OK | |
| 10 | ENC_SW | OK | ISR FALLING |
| 11 | TFT_MOSI | OK | SPI (shared z SD) |
| 12 | TFT_SCLK | OK | SPI (shared z SD) |
| 13 | TFT_MISO | OK | SPI (shared z SD) |
| 14 | TFT_CS | OK | |
| 15 | TFT_DC | OK | |
| 16 | TFT_RST | OK | |
| 17 | TFT_BL | OK | |
| 18 | RTC_SCL | OK | I2C Clock |
| 19 | BTN_PAUSE | OK | ISR FALLING |
| 20 | SEL_P3 | OK | |
| 21 | RELAY_1 | OK | |
| 26-34 | - | **WOLNE** | Dostępne na płytce DevKitC |
| 35 | LED_GREEN | OK | |
| 36 | LED_RED | OK | |
| 37 | LED_YELLOW | OK | |
| 38 | RELAY_5 | OK | |
| 39 | RELAY_6 | OK | |
| 40 | BTN_START | OK | ISR FALLING |
| 41 | BTN_STOP | OK | ISR FALLING |
| 42 | E-STOP | ⚠️ | Zweryfikować edge (FALLING vs RISING) |
| 43-44 | USB | **ZAREZERWOWANE** | USB JTAG (nie używać!) |
| 45 | RELAY_4 | ⚠️ | **Strapping pin!** Wymaga pull-down |
| 46 | BUZZER | ⚠️ | Strapping pin, ale OK jako output |
| 47 | RELAY_2 | OK | |
| 48 | RELAY_3 | OK | |

### 5.2 Konflikty pinów

**Brak bezpośrednich konfliktów** - żaden pin nie jest użyty podwójnie. Potencjalne problemy:
- GPIO 45 (strapping) - wymaga pull-down
- GPIO 46 (strapping) - akceptowalny z pull-down
- SPI bus (11,12,13) - współdzielony z potencjalnym SD card

### 5.3 Propozycja pinu dla SD Card

Rekomendacja: **GPIO 1** lub **GPIO 2** jako `SD_CS`:
```cpp
#define SD_CS_PIN   1   // Chip Select karty SD
```

---

## 6. REKOMENDACJA: KARTA SD ZAMIAST LittleFS

### Argumenty ZA:
1. **Pojemność**: SD card (4-32GB) vs LittleFS (~3MB na 16MB flash po partycji)
2. **Łatwość eksportu**: Wyjmij kartę → czytnik w PC
3. **Trwałość**: Flash ESP32 ma limit cykli zapisu (~10K-100K), SD card ma więcej
4. **Zintegrowany czytnik**: Już masz go przy TFT - zero dodatkowego okablowania

### Argumenty PRZECIW:
1. **Złożoność**: Współdzielenie SPI z TFT wymaga mutex/semaphore
2. **Niezawodność**: Karta SD może być wyjęta podczas pracy
3. **Czas dostępu**: Inicjalizacja SD card (~100ms) wolniejsza niż LittleFS

### Rekomendacja:
**HYBRYDOWE** podejście:
- **LittleFS** → error log (szybki zapis, mały rozmiar, krytyczne)
- **SD Card** → raporty pracy + CSV export (duże pliki, eksport)
- Dodać sprawdzanie obecności karty SD przy starcie
- Fallback na LittleFS gdy brak karty SD

---

## 7. PLAN REMEDIACJI (priorytetyzowany)

### Faza 1 - KRYTYCZNE (natychmiast)
1. ✅ Dodać logikę cyklowania wzorców przerywanych w `updateGuns()`
2. ✅ Zweryfikować i naprawić edge E-STOP ISR (FALLING vs RISING)

### Faza 2 - POWAŻNE (przed wdrożeniem)
3. ✅ Dodać pull-down na GPIO 45 (hardware) lub zmienić pin RELAY_4
4. ✅ Zmienić `static const` na `extern const` w pins.h (dodać pins.cpp)
5. ✅ Dodać Basic Auth do krytycznych endpointów API
6. ✅ Zaimplementować obsługę SD card (pin CS, moduł, migracja raportów)

### Faza 3 - UMIARKOWANE (poprawa jakości)
7. Dodać `ERR_DEADMAN_TIMEOUT` do typów błędów
8. Naprawić logikę buzzera
9. Przenieść WiFi credentials do NVS
10. Dodać endpoint usuwania raportów
11. Zoptymalizować budowanie JSON (snprintf/ArduinoJson)

### Faza 4 - NISKIE (opcjonalne)
12. Wybrać jedno podejście NTP (configTime lub NTPClient)
13. Warunkowa kompilacja TFT (`#ifdef USE_TFT`)
14. Dodać CORS headers

---

## 8. PODSUMOWANIE

Kod v6.0.0 jest **znacząco lepszy** od monolitu v5.3.0 - modularna struktura, naprawione krytyczne bugi GPIO, poprawiony CSV parser, walidacja API. Architektura jest czytelna i łatwa do rozbudowy.

**Najważniejszy problem**: Brak logiki cyklowania wzorców przerywanych - bez tego maszyna maluje **wszystkie wzorce jako ciągłe**, co jest fundamentalnym błędem funkcjonalnym.

**Drugi priorytet**: Weryfikacja E-STOP edge i zabezpieczenie GPIO 45 (strapping pin) przed wdrożeniem na produkcję.

Po naprawieniu tych dwóch krytycznych problemów, system jest gotowy do testów produkcyjnych z uwzględnieniem rekomendacji z faz 2-4.

---

*Audyt wykonany: 2026-02-02 | Trassar-Painter v6.0.0 MODULAR PRODUCTION EDITION*
