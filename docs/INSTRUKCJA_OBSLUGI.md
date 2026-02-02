# INSTRUKCJA OBSŁUGI

## Trassar-Painter v7.0.0 - Komputer Malowarki Drogowej

---

## SPIS TREŚCI

1. [Opis systemu](#1-opis-systemu)
2. [Uruchomienie](#2-uruchomienie)
3. [Tryby pracy](#3-tryby-pracy)
4. [Wzorce malowania](#4-wzorce-malowania)
5. [Panel WWW](#5-panel-www)
6. [Kalibracja enkodera](#6-kalibracja-enkodera)
7. [Raporty pracy](#7-raporty-pracy)
8. [Karta SD](#8-karta-sd)
9. [System bezpieczeństwa](#9-system-bezpieczenstwa)
10. [Kontrola prędkości](#10-kontrola-predkosci)
11. [Tryb serwisowy](#11-tryb-serwisowy)
12. [Selektor P3](#12-selektor-p3)
13. [Start od przerwy](#13-start-od-przerwy)
14. [Aktualizacja OTA](#14-aktualizacja-ota)
15. [Rozwiązywanie problemów](#15-rozwiazywanie-problemow)

---

## 1. OPIS SYSTEMU

Trassar-Painter to profesjonalny komputer sterujący malowarką drogową. System automatycznie kontroluje 6 pistoletów malarskich zgodnie z wybranymi wzorcami oznakowania drogowego (norma polska).

### Główne funkcje:
- 15 wzorców malowania (P-1a do P-7d) zgodnych z normą
- 6 kanałów pistoletów (12cm i 24cm)
- Pomiar dystansu i prędkości (enkoder)
- **Kontrola prędkości** - malowanie tylko powyżej 3 km/h
- **Karta SD** - zapis raportów na kartę microSD
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
   - Test karty SD (jeśli włożona)
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

### Przejścia między trybami (v7.0.0 - jeden przycisk):
```
IDLE ──[START/PAUZA]──> WORKING ──[START/PAUZA]──> PAUSED ──[START/PAUZA]──> WORKING
  │                        │                          │
  │                        └──[STOP]──────────────────┘──> IDLE
  │
  ├──[MEASURE]──> MEASURING ──[STOP]──> IDLE
  ├──[SERVICE]──> SERVICE ──[STOP]──> IDLE
  ├──[MENU]──> MENU ──[opcja]──> ...
  └──[CALIBRATE]──> CALIBRATING ──[STOP]──> IDLE

Dowolny tryb ──[E-STOP]──> EMERGENCY ──[RESET]──> IDLE
```

> **UWAGA**: Przycisk START/PAUZA to jeden fizyczny przycisk, który przełącza tryby w zależności od aktualnego stanu.

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

### Cyklowanie wzorców przerywanych (v7.0.0):
Dla wzorców przerywanych (linia + przerwa) system automatycznie przełącza pistolety na podstawie przejechanego dystansu. Nie trzeba ręcznie włączać/wyłączać pistoletów.

---

## 5. PANEL WWW

Panel dostępny pod adresem **http://192.168.4.1** po połączeniu z WiFi "Trassar-Painter".

### Zakładka 1: Panel Główny
- Status systemu (tryb, wzorzec, prędkość, selektor P3)
- Wskaźnik prędkości (zielony/czerwony gdy < 3 km/h)
- Info o cyklu wzorca (linia/przerwa)
- Selektor P3 wirtualny (przycisk)
- Opcja "Start od przerwy"
- Wybór wzorca (15 przycisków)
- Przyciski sterujące: **START/PAUZA**, STOP, MENU, SERWIS
- Wizualizacja pistoletów na żywo
- Info o karcie SD (pojemność, wolne miejsce)

### Zakładka 2: Pomiar
- Tryb pomiaru dystansu (pistolety OFF)

### Zakładka 3: Kalibracja
- Procedura automatycznej kalibracji

### Zakładka 4: Raporty
- Lista zapisanych raportów
- Podgląd szczegółów raportu
- Eksport CSV
- **Usuwanie raportów** (pojedynczo lub wszystkie)

---

## 6. KALIBRACJA ENKODERA

### Procedura:
1. Wyznacz na podłożu odcinek dokładnie **10 metrów**
2. Ustaw maszynę na początku odcinka
3. Wciśnij **START KALIBRACJI** (panel WWW lub menu)
4. Jedź powoli (~5 km/h) w linii prostej
5. Zatrzymaj się dokładnie na końcu 10m
6. Wciśnij **STOP**
7. Wartość zapisana do pamięci NVS (przetrwa restart)

---

## 7. RAPORTY PRACY

### Jak działają raporty:
1. **Automatyczny start** - raport rozpoczyna się przy pierwszym START malowania
2. **Śledzenie** - system zlicza dystans per wzorzec
3. **Automatyczny zapis** - raport zapisuje się po STOP
4. **Obliczanie powierzchni** - `m2 = dystans × szerokość`

### Zapis:
- **Karta SD** (priorytet) - jeśli karta jest dostępna
- **LittleFS** (fallback) - pamięć wewnętrzna ESP32

### Usuwanie raportów (v7.0.0):
- W panelu WWW: przycisk "Usuń" przy każdym raporcie
- "Usuń wszystkie" - kasuje wszystkie raporty

---

## 8. KARTA SD

### Wymagania:
- Karta microSD (FAT32)
- Czytnik zintegrowany z modułem TFT ILI9341

### Automatyczne działanie:
- System wykrywa kartę SD przy starcie (self-test)
- Raporty automatycznie zapisywane na kartę w katalogu `/raporty/`
- Jeśli karta niedostępna - fallback na LittleFS
- Info o pojemności w panelu WWW

---

## 9. SYSTEM BEZPIECZEŃSTWA

### E-STOP (Awaryjne zatrzymanie)
- **Czerwony przycisk grzybkowy** NC (Normally Closed)
- Natychmiast wyłącza WSZYSTKIE pistolety
- System przechodzi w tryb EMERGENCY
- LED CZERWONY stały + 5 sygnałów dźwiękowych
- **Aby zresetować**: zwolnij E-STOP → potwierdź w panelu WWW (wymaga hasła)

### Watchdog (5s)
- Automatyczny restart po zawieszeniu systemu

### Heartbeat (1s)
- Pistolety OFF jeśli pętla główna nie odpowiada

### Deadman Switch (30s)
- Operator musi potwierdzić obecność co 30 sekund
- Potwierdzenie = wciśnięcie START/PAUZA lub przycisku enkodera
- Timeout = automatyczna PAUZA + alarm

---

## 10. KONTROLA PRĘDKOŚCI (v7.0.0)

### Minimalna prędkość malowania: 3 km/h

Ze względów bezpieczeństwa malowanie jest możliwe tylko podczas jazdy maszyny z prędkością większą niż 3 km/h.

- **Poniżej 3 km/h**: pistolety automatycznie wyłączone (nawet w trybie WORKING)
- **Powyżej 3 km/h**: pistolety działają normalnie wg wzorca
- Wskaźnik prędkości w panelu WWW zmienia kolor:
  - **Zielony**: prędkość wystarczająca
  - **Czerwony**: za wolno, pistolety OFF

---

## 11. TRYB SERWISOWY

Test pistoletów BEZ JAZDY.
1. Wciśnij **TRYB SERWISOWY** w panelu WWW
2. **Przytrzymaj** przycisk wzorca - pistolety się włączą
3. **Puść** przycisk - pistolety się wyłączą
4. Wciśnij **WYJDŹ** aby wrócić do IDLE

---

## 12. SELEKTOR P3

Odwraca parę pistoletów P1 ↔ P3 dla wzorców podwójnych (P-3a, P-3b, P-4).

Sterowanie:
1. **Przełącznik fizyczny** (GPIO 20)
2. **Przycisk w panelu WWW**

---

## 13. START OD PRZERWY

1. Zaznacz **"Start od przerwy"** w panelu WWW
2. Wciśnij **START/PAUZA**
3. Pistolety OFF przez długość przerwy wzorca
4. Po przejechaniu przerwy - pistolety włączają się automatycznie

---

## 14. AKTUALIZACJA OTA

1. Połącz komputer z WiFi "Trassar-Painter"
2. W PlatformIO: `pio run -t upload --upload-port Trassar-Painter`
3. Hasło OTA: **trassar2024**

---

## 15. ROZWIĄZYWANIE PROBLEMÓW

| Problem | Przyczyna | Rozwiązanie |
|---------|-----------|-------------|
| LED CZERWONY po starcie | E-STOP wciśnięty | Zwolnij E-STOP, resetuj w panelu |
| Brak malowania mimo WORKING | Prędkość < 3 km/h | Jedź szybciej |
| Brak dystansu | Enkoder niepodłączony | Sprawdź GPIO 8,9,10 |
| Brak karty SD | Karta nie wykryta | Sprawdź kartę FAT32 |
| Brak panelu WWW | WiFi AP nie działa | Restart ESP32 |
| Automatyczna pauza | Deadman timeout | Wciśnij przycisk co <30s |

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
| 12 | ERR_SPEED_TOO_LOW | Prędkość poniżej minimum |
| 13 | ERR_DEADMAN_TIMEOUT | Brak potwierdzenia operatora |
| 14 | ERR_SD_CARD_FAILED | Karta SD nie działa |

---

## DANE TECHNICZNE

| Parametr | Wartość |
|----------|---------|
| Mikrokontroler | ESP32-S3 (16MB Flash, 8MB PSRAM) |
| Firmware | v7.0.0 SD-SPEED EDITION |
| Wzorce | 15 (P-1a do P-7d) |
| Pistolety | 6 kanałów (12cm + 24cm) |
| Wyświetlacz | TFT ILI9341 2.8" (320×240) |
| Karta SD | microSD FAT32 (współdzielony SPI) |
| WiFi AP | Trassar-Painter / 12345678 |
| Panel WWW | http://192.168.4.1 |
| Min. prędkość | 3 km/h |
| OTA | Trassar-Painter / trassar2024 |
| Zasilanie | 5V / 2A minimum |

---

*Trassar-Painter v7.0.0 - SD-SPEED EDITION*
*Autor: Trassar251 | Data: 2026-02-02*
