# INSTRUKCJA OBSŁUGI

## Trassar-Painter v6.0.0 - Komputer Malowarki Drogowej

---

## SPIS TREŚCI

1. [Opis systemu](#1-opis-systemu)
2. [Uruchomienie](#2-uruchomienie)
3. [Tryby pracy](#3-tryby-pracy)
4. [Wzorce malowania](#4-wzorce-malowania)
5. [Panel WWW](#5-panel-www)
6. [Kalibracja enkodera](#6-kalibracja-enkodera)
7. [Raporty pracy](#7-raporty-pracy)
8. [System bezpieczeństwa](#8-system-bezpieczenstwa)
9. [Tryb serwisowy](#9-tryb-serwisowy)
10. [Selektor P3](#10-selektor-p3)
11. [Start od przerwy](#11-start-od-przerwy)
12. [Aktualizacja OTA](#12-aktualizacja-ota)
13. [Rozwiązywanie problemów](#13-rozwiazywanie-problemow)

---

## 1. OPIS SYSTEMU

Trassar-Painter to profesjonalny komputer sterujący malowarką drogową. System automatycznie kontroluje 6 pistoletów malarskich zgodnie z wybranymi wzorcami oznakowania drogowego (norma polska).

### Główne funkcje:
- 15 wzorców malowania (P-1a do P-7d) zgodnych z normą
- 6 kanałów pistoletów (12cm i 24cm)
- Pomiar dystansu i prędkości (enkoder)
- Automatyczna kalibracja (jedź 10m)
- Panel WWW przez WiFi (telefon/tablet)
- Raporty pracy z obliczaniem powierzchni m2
- Zegar RTC z synchronizacją NTP
- System bezpieczeństwa (E-STOP, Watchdog, Self-test)
- Aktualizacja firmware OTA (przez WiFi)

---

## 2. URUCHOMIENIE

### Sekwencja startu:
1. Podłącz zasilanie 5V
2. System wykonuje **Self-test** (diagnostyka ~2s):
   - Test RTC DS1307
   - Test LittleFS (pamięć plików)
   - Test enkodera
   - Test przekaźników (krótki puls na każdym)
   - Test E-STOP (czy zwolniony)
   - Test LEDów i buzzera
3. Łączenie z WiFi STA (synchronizacja czasu NTP)
4. Uruchomienie Access Point "Trassar-Painter"
5. Uruchomienie serwera WWW na porcie 80
6. **LED ZIELONY** = system gotowy

### Połączenie z panelem WWW:
1. Na telefonie/tablecie połącz się z WiFi: **Trassar-Painter**
2. Hasło: **12345678**
3. Otwórz przeglądarkę: **http://192.168.4.1**

---

## 3. TRYBY PRACY

| Tryb | LED | Opis |
|------|-----|------|
| **IDLE** | Zielony stały | Gotowy do pracy |
| **WORKING** | Zielony migający | Malowanie aktywne, pistolety wg wzorca |
| **PAUSED** | Żółty | Pauza, pistolety OFF, wzorzec zachowany |
| **MEASURING** | - | Pomiar dystansu bez malowania |
| **MENU** | - | Nawigacja menu joystickiem |
| **SERVICE** | - | Test pistoletów bez jazdy |
| **CALIBRATING** | - | Kalibracja automatyczna |
| **EMERGENCY** | Czerwony stały | E-STOP aktywny, wszystkie pistolety OFF |

### Przejścia między trybami:
```
IDLE ──[START]──> WORKING ──[PAUSE]──> PAUSED ──[PAUSE]──> WORKING
  │                  │                    │
  │                  └──[STOP]────────────┘──> IDLE
  │
  ├──[MEASURE]──> MEASURING ──[STOP]──> IDLE
  ├──[SERVICE]──> SERVICE ──[STOP]──> IDLE
  ├──[MENU]──> MENU ──[opcja]──> ...
  └──[CALIBRATE]──> CALIBRATING ──[STOP]──> IDLE

Dowolny tryb ──[E-STOP]──> EMERGENCY ──[RESET]──> IDLE
```

---

## 4. WZORCE MALOWANIA

### Lista 15 wzorców:

| Nr | Nazwa | Opis | Linia | Przerwa | Szer. | Pistolety |
|----|-------|------|-------|---------|-------|-----------|
| 0 | P-1a | Przerywana długa | 4m | 8m | 12cm | P2 |
| 1 | P-1b | Przerywana krótka | 2m | 4m | 12cm | P2 |
| 2 | P-1c | Wydzielająca | 2m | 2m | 12cm | P2 |
| 3 | P-1d | Prowadząca wąska | 1m | 1m | 12cm | P2 |
| 4 | P-1e | Prowadząca szeroka | 1m | 1m | 24cm | P4 |
| 5 | **P-2a** | **Ciągła wąska (domyślny)** | ciągła | - | 12cm | P2 |
| 6 | P-2b | Ciągła szeroka | ciągła | - | 24cm | P4 |
| 7 | P-3a | Przekraczalna długa | 4m | 2m | 12cm | P1+P3 |
| 8 | P-3b | Przekraczalna krótka | 1m | 1m | 12cm | P1+P3 |
| 9 | P-4 | Podwójna ciągła | ciągła | - | 24cm | P1+P3 |
| 10 | P-6 | Ostrzegawcza | 4m | 2m | 12cm | P5 |
| 11 | P-7a | Kraw. przeryw. szer. | 1m | 1m | 24cm | P6 |
| 12 | P-7b | Kraw. ciągła szer. | ciągła | - | 24cm | P6 |
| 13 | P-7c | Kraw. przeryw. wąska | 1m | 1m | 12cm | P5 |
| 14 | P-7d | Kraw. ciągła wąska | ciągła | - | 12cm | P5 |

### Oznaczenia pistoletów:
- **P1** (12cm) - oś jezdni
- **P2** (12cm) - oś jezdni
- **P3** (12cm) - oś jezdni
- **P4** (24cm) - oś jezdni szeroki
- **P5** (12cm K) - krawędziowy
- **P6** (24cm K) - krawędziowy szeroki

---

## 5. PANEL WWW

Panel dostępny pod adresem **http://192.168.4.1** po połączeniu z WiFi "Trassar-Painter".

### Zakładka 1: Panel Główny
- Status systemu (tryb, wzorzec, selektor P3)
- Pomiary: dystans, prędkość, impulsy enkodera
- Selektor P3 wirtualny (przycisk)
- Opcja "Start od przerwy"
- Wybór wzorca (15 przycisków)
- Przyciski sterujące: START, STOP, PAUSE, MENU, SERWIS
- Wizualizacja pistoletów na żywo

### Zakładka 2: Pomiar
- Tryb pomiaru dystansu (pistolety OFF)
- START POMIARU / STOP

### Zakładka 3: Kalibracja
- Procedura automatycznej kalibracji
- START KALIBRACJI / STOP
- Podgląd impulsów i aktualnej kalibracji

### Zakładka 4: Raporty
- Status bieżącego raportu
- Lista zapisanych raportów
- Podgląd szczegółów raportu
- Eksport CSV

---

## 6. KALIBRACJA ENKODERA

### Procedura kalibracji automatycznej:
1. Wyznacz na podłożu odcinek dokładnie **10 metrów**
2. Ustaw maszynę na początku odcinka
3. Wciśnij **START KALIBRACJI** (panel WWW lub menu)
4. Jedź powoli (~5 km/h) w linii prostej
5. Zatrzymaj się dokładnie na końcu 10m
6. Wciśnij **STOP**
7. System automatycznie obliczy: `kalibracja = impulsy / 10`
8. Wartość zapisana do pamięci NVS (przetrwa restart)

### Wskazówki:
- Kalibruj na tej samej powierzchni co praca (asfalt, beton)
- Jedź ze stałą prędkością
- Powtórz kalibrację po zmianie koła/opony

---

## 7. RAPORTY PRACY

### Jak działają raporty:
1. **Automatyczny start** - raport rozpoczyna się przy pierwszym START malowania
2. **Śledzenie** - system zlicza dystans per wzorzec
3. **Automatyczny zapis** - raport zapisuje się po STOP
4. **Obliczanie powierzchni** - `m2 = dystans × szerokość`

### Eksport CSV:
1. Przejdź do zakładki **Raporty**
2. Kliknij raport z listy
3. Kliknij przycisk **CSV**
4. Plik zostanie pobrany

### Format raportu:
```
Data: 2026-02-02
Rozpoczęcie: 08:00:00
Zakończenie: 12:30:00
Czas pracy: 16200 s (270 min)

POWIERZCHNIA WYMALOWANA [m2]:
P-2a (Ciągła wąska): 120.50 m2 (dystans: 1004.17 m)
P-1a (Przerywana długa): 45.00 m2 (dystans: 375.00 m)

SUMA: 165.50 m2
```

---

## 8. SYSTEM BEZPIECZEŃSTWA

### E-STOP (Awaryjne zatrzymanie)
- **Czerwony przycisk grzybkowy** na maszynie
- Natychmiast wyłącza WSZYSTKIE pistolety
- System przechodzi w tryb EMERGENCY
- LED CZERWONY stały + 5 sygnałów dźwiękowych
- **Aby zresetować**: zwolnij E-STOP → wciśnij START

### Watchdog (5s)
- Jeśli system się zawiesi, automatyczny restart po 5 sekundach
- Chroni przed zablokowaniem pistoletu w stanie ON

### Heartbeat (1s)
- Jeśli pętla główna przestanie odpowiadać, pistolety zostaną wyłączone
- Dodatkowa warstwa ochrony

### Deadman Switch (30s)
- Operator musi potwierdzić swoją obecność co 30 sekund
- Potwierdzenie = wciśnięcie START, PAUSE lub przycisku enkodera
- Timeout = automatyczna PAUZA + alarm

### Encoder Health
- **Stall detection**: jeśli enkoder nie zlicza impulsów przez 5s podczas pracy
- **Disconnect detection**: jeśli po 10s pracy nadal 0 impulsów
- Żółte/czerwone ostrzeżenie + buzzer

### Speed Validation
- Limit 0-25 km/h
- Nierealistyczna prędkość = automatyczna PAUZA + alarm

### Self-test (przy starcie)
- Test RTC, LittleFS, enkodera, przekaźników, E-STOP, GPIO
- Wynik: PASSED (zielony) lub WARNINGS (żółty)

### Error Log
- 50 ostatnich błędów w pamięci
- Zapis do pliku `/errors.log`
- Dostępny przez API: `/api/safety/errors`

---

## 9. TRYB SERWISOWY

Tryb serwisowy umożliwia testowanie pistoletów BEZ JAZDY.

### Uruchomienie:
1. Wciśnij **TRYB SERWISOWY** w panelu WWW
2. **Przytrzymaj** przycisk wzorca - pistolety się włączą
3. **Puść** przycisk - pistolety się wyłączą
4. Wciśnij **WYJDŹ** aby wrócić do IDLE

### Zastosowanie:
- Sprawdzanie czy pistolety działają
- Test dysz
- Diagnostyka przekaźników

---

## 10. SELEKTOR P3

Selektor P3 odwraca parę pistoletów P1 ↔ P3 dla wzorców podwójnych:
- **P-3a** (Przekraczalna długa)
- **P-3b** (Przekraczalna krótka)
- **P-4** (Podwójna ciągła)

### Dwa sposoby sterowania:
1. **Przełącznik fizyczny** (GPIO 20) - LOW=normalne, HIGH=odwrócone
2. **Przycisk w panelu WWW** - kliknij aby przełączyć

Oba działają razem (OR logiczny). Jeśli którykolwiek jest ODWRÓCONY,
para P1↔P3 jest zamieniona.

---

## 11. START OD PRZERWY

Funkcja "Start od przerwy" pozwala rozpocząć malowanie od przerwy wzorca.

### Jak to działa:
1. Zaznacz **"Start od przerwy"** w panelu WWW
2. Wciśnij **START**
3. Pistolety są OFF przez długość przerwy wzorca
4. Po przejechaniu przerwy - pistolety włączają się automatycznie
5. Pasek postępu pokazuje ile do końca przerwy

### Zastosowanie:
- Synchronizacja z istniejącym oznakowaniem
- Kontynuacja malowania po przerwie

---

## 12. AKTUALIZACJA OTA

Aktualizacja firmware przez WiFi (bez kabla USB).

### Procedura:
1. Połącz komputer z WiFi "Trassar-Painter"
2. W panelu WWW: MENU → Aktualizacja OTA
3. W PlatformIO: `pio run -t upload --upload-port Trassar-Painter`
4. Hasło OTA: **trassar2024**

---

## 13. ROZWIĄZYWANIE PROBLEMÓW

| Problem | Przyczyna | Rozwiązanie |
|---------|-----------|-------------|
| LED CZERWONY po starcie | E-STOP wciśnięty | Zwolnij E-STOP, wciśnij START |
| Brak dystansu | Enkoder niepodłączony | Sprawdź połączenia GPIO 8,9,10 |
| Prędkość 0 | Brak impulsów | Kalibracja + sprawdź enkoder |
| Zły dystans | Zła kalibracja | Wykonaj kalibrację (10m) |
| Brak panelu WWW | WiFi AP nie działa | Restart ESP32 |
| Zły czas | NTP nie zsynchronizowany | Sprawdź WiFi STA + internet |
| Buzzer ciągły | Wielokrotne błędy | Sprawdź `/api/safety/errors` |
| Automatyczna pauza | Deadman timeout | Wciśnij przycisk co <30s |
| Restart ESP32 | Watchdog timeout | Sprawdź error log |
| Brak raportów | LittleFS pełny | Usuń stare raporty |

### Kody błędów:
| Kod | Typ | Opis |
|-----|-----|------|
| 1 | ERR_ESTOP_PRESSED | E-STOP wciśnięty |
| 2 | ERR_WATCHDOG_TIMEOUT | System zawieszony |
| 3 | ERR_ENCODER_STALL | Enkoder zatrzymany |
| 4 | ERR_ENCODER_DISCONNECTED | Enkoder odłączony |
| 5 | ERR_SPEED_INVALID | Prędkość poza zakresem |
| 6 | ERR_GUN_FEEDBACK | Błąd przekaźnika |
| 7 | ERR_RTC_FAILED | RTC nie działa |
| 8 | ERR_FILESYSTEM_FULL | Brak miejsca na raporty |
| 9 | ERR_CALIBRATION_DRIFT | Kalibracja dryfuje |
| 10 | ERR_HEARTBEAT_TIMEOUT | System nie odpowiada |
| 11 | ERR_SELF_TEST_FAILED | Self-test nie przeszedł |

---

## DANE TECHNICZNE

| Parametr | Wartość |
|----------|---------|
| Mikrokontroler | ESP32-S3 (16MB Flash, 8MB PSRAM) |
| Wzorce | 15 (P-1a do P-7d) |
| Pistolety | 6 kanałów (12cm + 24cm) |
| Wyświetlacz | TFT ILI9341 2.8" (320×240) |
| WiFi AP | Trassar-Painter / 12345678 |
| Panel WWW | http://192.168.4.1 |
| OTA | Trassar-Painter / trassar2024 |
| Prędkość max | 25 km/h |
| Kalibracja | automatyczna (10m) |
| Raportowanie | automatyczne (m2) |
| Zasilanie | 5V / 2A minimum |

---

*Trassar-Painter v6.0.0 - Modular Production Edition*
*Autor: Trassar251 | Data: 2026-02-02*
