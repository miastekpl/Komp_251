# INSTRUKCJA OBSŁUGI

## Trassar-Painter v7.0.0 SD-SPEED EDITION
### Komputer Malowarki Drogowej

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
9. [System bezpieczeństwa](#9-system-bezpieczeństwa)
10. [Kontrola prędkości](#10-kontrola-prędkości)
11. [Tryb serwisowy](#11-tryb-serwisowy)
12. [Selektor P3](#12-selektor-p3)
13. [Start od przerwy](#13-start-od-przerwy)
14. [Tryb pomiaru dystansu](#14-tryb-pomiaru-dystansu)
15. [Aktualizacja OTA](#15-aktualizacja-ota)
16. [Przykłady pracy](#16-przykłady-pracy)
17. [Rozwiązywanie problemów](#17-rozwiązywanie-problemów)
18. [Dane techniczne](#18-dane-techniczne)

---

## 1. OPIS SYSTEMU

Trassar-Painter to profesjonalny komputer sterujący malowarką drogową. System automatycznie kontroluje 6 pistoletów malarskich zgodnie z wybranymi wzorcami oznakowania drogowego (polska norma).

### 1.1 Główne funkcje

| Funkcja | Opis |
|---------|------|
| **15 wzorców malowania** | P-1a do P-7d zgodnych z normą PN |
| **6 kanałów pistoletów** | P1-P4 (oś jezdni), P5-P6 (krawędź) |
| **Enkoder pomiarowy** | Precyzyjny pomiar dystansu i prędkości |
| **Kontrola prędkości** | Malowanie tylko powyżej 3 km/h (bezpieczeństwo) |
| **Automatyczne cyklowanie** | Wzorce przerywane: linia/przerwa wg dystansu |
| **Karta SD** | Zapis raportów na kartę microSD |
| **Panel WWW** | Sterowanie przez WiFi (telefon/tablet) |
| **Raporty pracy** | Obliczanie powierzchni m² + eksport CSV |
| **RTC + NTP** | Zegar czasu rzeczywistego z synchronizacją |
| **System bezpieczeństwa** | E-STOP, Watchdog, Deadman, Self-test |
| **Aktualizacja OTA** | Firmware przez WiFi |

### 1.2 Rozmieszczenie pistoletów

```
Kierunek jazdy ───────────────────────────────────────────>

  ┌─────────────────────────────────────────────────────────┐
  │                        MALOWARKA                        │
  │                                                         │
  │   P5 (12cm)   P1 (12cm)   P2 (12cm)   P3 (12cm)   P6 (24cm)
  │      │            │           │           │           │
  └──────┼────────────┼───────────┼───────────┼───────────┼─┘
         │            │           │           │           │
         ▼            ▼           ▼           ▼           ▼
     ┌───────┐    ┌───────┐   ┌───────┐   ┌───────┐   ┌───────┐
     │KRAWĘDŹ│    │  OŚ   │   │  OŚ   │   │  OŚ   │   │KRAWĘDŹ│
     │ LEWA  │    │       │   │(DOMYŚLNY)│ │       │   │ PRAWA │
     └───────┘    └───────┘   └───────┘   └───────┘   └───────┘

  P4 (24cm) = P2 + prawa dyza = szeroka linia 24cm na osi
```

### 1.3 Elementy sterowania

```
┌──────────────────────────────────────────────────────────────┐
│                    PANEL STEROWANIA                          │
│                                                              │
│  ┌─────────┐   ┌─────────┐   ┌──────────────┐   ┌─────────┐ │
│  │ E-STOP  │   │ START/  │   │    STOP      │   │SELEKTOR │ │
│  │(czerwony│   │ PAUZA   │   │   (czarny)   │   │   P3    │ │
│  │grzybek) │   │(zielony)│   │              │   │(2-poz.) │ │
│  └─────────┘   └─────────┘   └──────────────┘   └─────────┘ │
│                                                              │
│  ┌──────────────────────┐   ┌────────────────────────────┐  │
│  │      JOYSTICK        │   │      ENKODER RĘCZNY        │  │
│  │  ↑ góra (menu)       │   │  Obrót = przewijanie       │  │
│  │← lewo    prawo →     │   │  Wciśnięcie = zatwierdź    │  │
│  │  ↓ dół               │   └────────────────────────────┘  │
│  │  Wciśnij = OK        │                                   │
│  └──────────────────────┘                                   │
│                                                              │
│  LED: 🟢 Zielony (OK)  🔴 Czerwony (BŁĄD)  🟡 Żółty (PAUZA) │
└──────────────────────────────────────────────────────────────┘
```

---

## 2. URUCHOMIENIE

### 2.1 Sekwencja startu

```
ZASILANIE ON
     │
     ▼
┌─────────────────────────────────────────────────────────────┐
│                    SELF-TEST (~2 sekundy)                   │
├─────────────────────────────────────────────────────────────┤
│  ✓ Test RTC DS1307 (zegar)                                  │
│  ✓ Test LittleFS (pamięć plików)                            │
│  ✓ Test karty SD (jeśli włożona)                            │
│  ✓ Test enkodera (podłączenie)                              │
│  ✓ Test przekaźników (krótki puls na każdym)                │
│  ✓ Test E-STOP (czy zwolniony)                              │
│  ✓ Test LEDów i buzzera                                     │
└─────────────────────────────────────────────────────────────┘
     │
     ▼
┌─────────────────────────────────────────────────────────────┐
│  WiFi STA: Łączenie z siecią (synchronizacja NTP)           │
│  WiFi AP: Uruchomienie "Trassar-Painter" (hasło: 12345678)  │
│  Serwer WWW: Port 80                                        │
└─────────────────────────────────────────────────────────────┘
     │
     ▼
   🟢 LED ZIELONY = SYSTEM GOTOWY (tryb IDLE)
```

### 2.2 Połączenie z panelem WWW

1. Na telefonie/tablecie wejdź w ustawienia WiFi
2. Połącz się z siecią: **Trassar-Painter**
3. Hasło: **12345678**
4. Otwórz przeglądarkę i wpisz: **http://192.168.4.1**

---

## 3. TRYBY PRACY

### 3.1 Tabela trybów

| Tryb | LED | Opis | Pistolety |
|------|-----|------|-----------|
| **IDLE** | 🟢 Zielony stały | Gotowy do pracy | OFF |
| **WORKING** | 🟢 Zielony migający | Malowanie aktywne | Wg wzorca (jeśli v > 3 km/h) |
| **PAUSED** | 🟡 Żółty stały | Pauza, wzorzec zachowany | OFF |
| **MEASURING** | 🟢 Zielony | Pomiar dystansu bez malowania | OFF |
| **SERVICE** | 🟡 Żółty migający | Test pistoletów bez jazdy | Ręcznie |
| **CALIBRATING** | 🟢/🟡 naprzemiennie | Kalibracja enkodera | OFF |
| **EMERGENCY** | 🔴 Czerwony stały | E-STOP wciśnięty! | OFF (wszystkie!) |

### 3.2 Diagram przejść między trybami

```
                              ┌─────────────────┐
                              │    EMERGENCY    │
                              │  🔴 E-STOP ON   │
                              └────────▲────────┘
                                       │
                    [E-STOP wciśnięty] │ [z dowolnego trybu]
                                       │
┌──────────┐  [START/PAUZA]  ┌─────────┴───────┐  [START/PAUZA]  ┌──────────┐
│   IDLE   │ ───────────────>│    WORKING      │<───────────────>│  PAUSED  │
│ 🟢 stały │                 │ 🟢 migający     │                 │ 🟡 stały │
└────┬─────┘                 └────────┬────────┘                 └────┬─────┘
     │                                │                               │
     │ [MEASURE]                      │ [STOP]                        │ [STOP]
     ▼                                ▼                               ▼
┌──────────┐                    ┌──────────┐                    ┌──────────┐
│MEASURING │ ──────[STOP]──────>│   IDLE   │<──────[STOP]──────│  PAUSED  │
└──────────┘                    └──────────┘                    └──────────┘

     │ [SERVICE]                      │ [CALIBRATE]
     ▼                                ▼
┌──────────┐                    ┌──────────────┐
│ SERVICE  │ ──────[STOP]──────>│ CALIBRATING  │ ──────[STOP]──────> IDLE
└──────────┘                    └──────────────┘
```

### 3.3 Jeden przycisk START/PAUZA (nowość v7.0.0)

System używa **jednego fizycznego przycisku** do startu i pauzy:

| Aktualny tryb | Po wciśnięciu START/PAUZA |
|---------------|---------------------------|
| IDLE | → WORKING (rozpocznij malowanie) |
| WORKING | → PAUSED (wstrzymaj) |
| PAUSED | → WORKING (wznów) |

---

## 4. WZORCE MALOWANIA

### 4.1 Lista 15 wzorców

| Nr | Nazwa | Opis | Linia | Przerwa | Szer. | Pistolety |
|----|-------|------|-------|---------|-------|-----------|
| 0 | P-1a | Przerywana długa | 4m | 8m | 12cm | P2 |
| 1 | P-1b | Przerywana krótka | 2m | 4m | 12cm | P2 |
| 2 | P-1c | Wydzielająca | 2m | 2m | 12cm | P2 |
| 3 | P-1d | Prowadząca wąska | 1m | 1m | 12cm | P2 |
| 4 | P-1e | Prowadząca szeroka | 1m | 1m | 24cm | P4 |
| 5 | **P-2a** | **Ciągła wąska (DOMYŚLNY)** | ciągła | - | 12cm | P2 |
| 6 | P-2b | Ciągła szeroka | ciągła | - | 24cm | P4 |
| 7 | P-3a | Przekraczalna długa | 4m | 2m | 12cm | P1+P3 |
| 8 | P-3b | Przekraczalna krótka | 1m | 1m | 12cm | P1+P3 |
| 9 | P-4 | Podwójna ciągła | ciągła | - | 24cm | P1+P3 |
| 10 | P-6 | Ostrzegawcza | 4m | 2m | 12cm | P5 |
| 11 | P-7a | Kraw. przeryw. szer. | 1m | 1m | 24cm | P6 |
| 12 | P-7b | Kraw. ciągła szer. | ciągła | - | 24cm | P6 |
| 13 | P-7c | Kraw. przeryw. wąska | 1m | 1m | 12cm | P5 |
| 14 | P-7d | Kraw. ciągła wąska | ciągła | - | 12cm | P5 |

### 4.2 Wizualizacja wzorców

```
P-1a: Przerywana długa (4m linia + 8m przerwa)
═════════════════          ═════════════════          ═════════════════
     4 metry       8 metrów      4 metry       8 metrów      4 metry

P-1b: Przerywana krótka (2m + 4m)
═══════════      ═══════════      ═══════════      ═══════════
   2m      4m       2m      4m       2m      4m       2m

P-1c: Wydzielająca (2m + 2m)
═══════════   ═══════════   ═══════════   ═══════════
   2m    2m      2m    2m      2m    2m      2m

P-1d/P-1e: Prowadząca (1m + 1m)
═════   ═════   ═════   ═════   ═════   ═════   ═════
 1m  1m  1m  1m  1m  1m  1m  1m  1m  1m  1m  1m  1m

P-2a/P-2b: Ciągła
════════════════════════════════════════════════════════════

P-3a/P-3b/P-4: Podwójna (P1 + P3)
═══════════   ═══════════   ═══════════   (P-3a/P-3b)
═══════════   ═══════════   ═══════════
lub ciągła:
════════════════════════════════════════ (P-4)
════════════════════════════════════════
```

### 4.3 Automatyczne cyklowanie wzorców przerywanych (v7.0.0)

Dla wzorców z przerwą (P-1a, P-1b, P-1c, P-1d, P-1e, P-3a, P-3b, P-6, P-7a, P-7c) system **automatycznie** przełącza pistolety między fazą "linia" i "przerwa" na podstawie przejechanego dystansu.

```
Przykład P-1a (4m linia + 8m przerwa):

Dystans:  0m ──────── 4m ──────── 12m ──────── 16m ──────── 24m
Faza:     |  LINIA   |   PRZERWA   |   LINIA   |   PRZERWA   |
Pistolety:|   ON     |     OFF     |    ON     |     OFF     |
          ════════════             ════════════
```

**Wskaźnik w panelu WWW**: Pokazuje aktualną fazę (LINIA / PRZERWA) oraz postęp w cyklu.

---

## 5. PANEL WWW

### 5.1 Dostęp

| Parametr | Wartość |
|----------|---------|
| Sieć WiFi | Trassar-Painter |
| Hasło WiFi | 12345678 |
| Adres panelu | http://192.168.4.1 |
| Login (dla E-STOP reset) | admin |
| Hasło (dla E-STOP reset) | trassar251 |

### 5.2 Zakładki panelu

#### Zakładka: PANEL GŁÓWNY
```
┌─────────────────────────────────────────────────────────────────────┐
│  TRASSAR-PAINTER v7.0.0                              🟢 SYSTEM OK   │
├─────────────────────────────────────────────────────────────────────┤
│  Tryb: IDLE          Wzorzec: P-2a (Ciągła wąska)                   │
│  Prędkość: 0.0 km/h  🔴 (za wolno!)   Dystans: 0.00 m              │
│  Cykl: -- (wzorzec ciągły)                                          │
│  Selektor P3: [NORMALNY]     □ Start od przerwy                    │
├─────────────────────────────────────────────────────────────────────┤
│  WZORCE:                                                            │
│  [P-1a] [P-1b] [P-1c] [P-1d] [P-1e]                                │
│  [P-2a] [P-2b] [P-3a] [P-3b] [P-4 ]                                │
│  [P-6 ] [P-7a] [P-7b] [P-7c] [P-7d]                                │
├─────────────────────────────────────────────────────────────────────┤
│  STEROWANIE:                                                        │
│  [ START/PAUZA ]  [ STOP ]  [ SERWIS ]  [ POMIAR ]                 │
├─────────────────────────────────────────────────────────────────────┤
│  PISTOLETY:                                                         │
│  P1: ○  P2: ○  P3: ○  P4: ○  P5: ○  P6: ○                          │
├─────────────────────────────────────────────────────────────────────┤
│  KARTA SD: Dostępna | 14.2 GB / 16.0 GB wolne                       │
└─────────────────────────────────────────────────────────────────────┘
```

#### Zakładka: POMIAR
- Uruchamia tryb MEASURING (pomiar dystansu bez malowania)
- Wyświetla: aktualny dystans, prędkość

#### Zakładka: KALIBRACJA
- Procedura kalibracji enkodera
- Przycisk START/STOP kalibracji

#### Zakładka: RAPORTY
- Lista zapisanych raportów
- Podgląd szczegółów
- Eksport do CSV
- Usuwanie (pojedyncze / wszystkie)

---

## 6. KALIBRACJA ENKODERA

### 6.1 Kiedy kalibrować?

- Przy pierwszym uruchomieniu
- Po wymianie koła pomiarowego
- Gdy zauważysz nieprawidłowy pomiar dystansu
- Po zmianie rozmiaru opon

### 6.2 Procedura kalibracji

```
1. PRZYGOTOWANIE
   ├── Wyznacz na podłożu odcinek dokładnie 10 metrów
   ├── Oznacz wyraźnie START i KONIEC
   └── Ustaw maszynę kołem pomiarowym na linii START

2. START KALIBRACJI
   ├── Panel WWW → Kalibracja → [START KALIBRACJI]
   └── lub Joystick → Menu → Kalibracja → [OK]

3. PRZEJAZD
   ├── Jedź POWOLI (~5 km/h) w linii prostej
   ├── Trzymaj równą prędkość
   └── Nie zatrzymuj się w trakcie

4. KONIEC
   ├── Zatrzymaj się DOKŁADNIE na linii końcowej (10m)
   └── Wciśnij [STOP] w panelu lub przycisk STOP

5. ZAPIS
   └── Wartość automatycznie zapisana do pamięci NVS
       (przetrwa restart i wyłączenie zasilania)
```

---

## 7. RAPORTY PRACY

### 7.1 Automatyczne tworzenie

```
START MALOWANIA
     │
     ├── System tworzy nowy raport
     ├── Zapisuje datę/godzinę startu
     │
     ▼
PRACA (tryb WORKING)
     │
     ├── Zlicza dystans per wzorzec
     ├── Oblicza powierzchnię: m² = dystans × szerokość
     │
     ▼
STOP (lub koniec dnia)
     │
     ├── Zapisuje godzinę zakończenia
     ├── Oblicza czas pracy
     └── Zapisuje raport na kartę SD (lub LittleFS)
```

### 7.2 Struktura raportu

| Pole | Opis |
|------|------|
| ID | Unikalny numer raportu |
| Data rozpoczęcia | RRRR-MM-DD |
| Godzina start | HH:MM:SS |
| Godzina koniec | HH:MM:SS |
| Czas pracy | W sekundach |
| Powierzchnia per wzorzec | m² dla każdego z 15 wzorców |
| Powierzchnia całkowita | Suma wszystkich m² |

### 7.3 Eksport CSV

Format: `raport_ID.csv`
```csv
Wzorzec,Dystans_m,Szerokosc_cm,Powierzchnia_m2
P-1a,150.5,12,18.06
P-2a,2340.2,12,280.82
P-7b,500.0,24,120.00
SUMA,,,418.88
```

---

## 8. KARTA SD

### 8.1 Wymagania

| Parametr | Wymaganie |
|----------|-----------|
| Typ karty | microSD |
| System plików | FAT32 |
| Pojemność | 1 GB - 32 GB |
| Prędkość | Class 4 lub wyższa |

### 8.2 Struktura katalogów

```
/
├── raporty/
│   ├── raport_001.json
│   ├── raport_002.json
│   └── ...
└── logi/
    ├── errors_2026-02-01.log
    └── ...
```

### 8.3 Automatyczne działanie

1. **Przy starcie**: System sprawdza kartę SD w self-teście
2. **Zapis raportów**: Priorytetowo na kartę SD
3. **Fallback**: Jeśli brak karty → zapis do LittleFS (pamięć wewnętrzna)
4. **Panel WWW**: Pokazuje status karty i wolne miejsce

---

## 9. SYSTEM BEZPIECZEŃSTWA

### 9.1 E-STOP (Awaryjne zatrzymanie)

```
PRZYCISK E-STOP (czerwony grzybek, NC - Normally Closed)
          │
          ▼
┌─────────────────────────────────────────────────────────────────────┐
│  WCIŚNIĘCIE E-STOP:                                                 │
│  1. Natychmiast wyłącza WSZYSTKIE przekaźniki (pistolety OFF)       │
│  2. System przechodzi w tryb EMERGENCY                              │
│  3. LED czerwony świeci stale                                       │
│  4. 5 sygnałów dźwiękowych buzzerem                                 │
│  5. Blokada wszystkich operacji                                     │
└─────────────────────────────────────────────────────────────────────┘

RESET E-STOP:
1. Odblokuj (przekręć) czerwony grzybek E-STOP
2. Wejdź w panel WWW → pojawi się baner "TRYB AWARYJNY"
3. Kliknij [RESETUJ E-STOP]
4. Podaj hasło: admin / trassar251
5. System wraca do trybu IDLE
```

### 9.2 Watchdog (5s)

- Automatyczny restart ESP32 jeśli pętla główna się zawiesi
- Chroni przed "zamrożeniem" systemu

### 9.3 Heartbeat (1s)

- Sygnał życia wysyłany co sekundę
- Jeśli brak sygnału → pistolety OFF
- Chroni przed błędem w kodzie sterowania

### 9.4 Deadman Switch (30s)

```
PODCZAS MALOWANIA (WORKING):
          │
          ├── Co 30 sekund wymaga potwierdzenia obecności operatora
          │
          ├── Potwierdzenie = wciśnięcie dowolnego przycisku:
          │   - START/PAUZA
          │   - Przycisk enkodera
          │   - Przycisk joysticka
          │
          └── Brak potwierdzenia → automatyczna PAUZA + alarm
```

---

## 10. KONTROLA PRĘDKOŚCI (v7.0.0)

### 10.1 Minimalna prędkość malowania

| Parametr | Wartość |
|----------|---------|
| **Próg minimalny** | **3 km/h** |
| Poniżej progu | Pistolety automatycznie OFF |
| Powyżej progu | Pistolety działają wg wzorca |

### 10.2 Dlaczego kontrola prędkości?

Ze względów **bezpieczeństwa** malowanie jest możliwe tylko podczas jazdy:
- Zapobiega przypadkowemu malowaniu na postoju
- Chroni przed rozlaniem farby przy ruszaniu/zatrzymywaniu
- Zapewnia równomierne nanoszenie farby

### 10.3 Wskaźnik w panelu WWW

```
Prędkość: 5.2 km/h  🟢   ← wystarczająca, malowanie aktywne
Prędkość: 2.1 km/h  🔴   ← za wolno! pistolety wyłączone
Prędkość: 0.0 km/h  🔴   ← postój, pistolety wyłączone
```

---

## 11. TRYB SERWISOWY

### 11.1 Przeznaczenie

Test pistoletów **BEZ JAZDY** - do sprawdzenia działania dysz i przekaźników.

### 11.2 Obsługa

```
1. Panel WWW → [TRYB SERWISOWY]
   lub Menu → Serwis → [OK]

2. Na ekranie pojawią się przyciski wzorców

3. PRZYTRZYMAJ przycisk wzorca:
   - Pistolety przypisane do wzorca włączą się
   - Farba leci (UWAGA!)

4. PUŚĆ przycisk:
   - Pistolety się wyłączą

5. [WYJDŹ] → powrót do trybu IDLE
```

### 11.3 Uwagi

- W trybie serwisowym **nie działa kontrola prędkości** (można testować na postoju)
- Raport nie jest tworzony
- Używaj ostrożnie - farba leci natychmiast!

---

## 12. SELEKTOR P3

### 12.1 Funkcja

Odwraca parę pistoletów P1 ↔ P3 dla wzorców podwójnych (P-3a, P-3b, P-4).

### 12.2 Zastosowanie

```
Standardowo (P3 = NORMALNY):
  Lewy pas → P1
  Prawy pas → P3

Odwrócony (P3 = ODWRÓCONY):
  Lewy pas → P3
  Prawy pas → P1

Przydatne gdy malujemy w przeciwnym kierunku lub zmieniamy stronę drogi.
```

### 12.3 Sterowanie

1. **Przełącznik fizyczny** (GPIO 20) - dwupozycyjny
2. **Przycisk w panelu WWW** - kliknij [SELEKTOR P3]

---

## 13. START OD PRZERWY

### 13.1 Funkcja

Pozwala rozpocząć malowanie od fazy "przerwa" zamiast "linia". Przydatne gdy dołączamy do istniejącej linii przerywanej.

### 13.2 Obsługa

```
1. Panel WWW → zaznacz checkbox: ☑ Start od przerwy

2. Wybierz wzorzec przerywany (np. P-1a)

3. Wciśnij [START/PAUZA]

4. Jedź do przodu:
   - Pistolety pozostają OFF przez długość przerwy wzorca
   - Panel pokazuje postęp: "Przerwa: 2.5m / 8.0m"
   - Po przejechaniu przerwy → pistolety włączają się automatycznie
   - Dalej działa normalne cyklowanie linia/przerwa
```

### 13.3 Przykład

```
Wzorzec P-1a (4m linia + 8m przerwa), Start od przerwy:

Dystans:  0m ──────── 8m ──────── 12m ──────── 20m ──────── 24m
Faza:     |   PRZERWA   |  LINIA  |   PRZERWA   |  LINIA  |
Pistolety:|     OFF     |   ON    |     OFF     |   ON    |
                        ════════════             ════════════
```

---

## 14. TRYB POMIARU DYSTANSU

### 14.1 Funkcja

Pomiar przejechanego dystansu **bez malowania**. Przydatne do:
- Sprawdzenia długości odcinka przed malowaniem
- Weryfikacji kalibracji enkodera
- Planowania pracy

### 14.2 Obsługa

```
1. Panel WWW → [POMIAR] lub Menu → Pomiar → [OK]

2. System przechodzi w tryb MEASURING
   - LED zielony
   - Pistolety OFF (zawsze!)

3. Jedź i obserwuj:
   - Dystans: aktualizowany na żywo
   - Prędkość: aktualna prędkość

4. [STOP] → zapisz wynik, powrót do IDLE
```

### 14.3 Uwagi

- Wynik pomiaru nie jest zapisywany do raportu
- Można przerwać w dowolnym momencie
- Prędkość minimalna nie obowiązuje (pistolety i tak są OFF)

---

## 15. AKTUALIZACJA OTA

### 15.1 Wymagania

- Komputer z PlatformIO
- Połączenie WiFi z siecią "Trassar-Painter"

### 15.2 Procedura

```bash
# W terminalu PlatformIO:
pio run -t upload --upload-port Trassar-Painter

# Hasło OTA: trassar2024
```

### 15.3 Uwagi

- Aktualizacja trwa około 30-60 sekund
- Nie wyłączaj zasilania podczas aktualizacji!
- Po aktualizacji system automatycznie się zrestartuje

---

## 16. PRZYKŁADY PRACY

### PRZYKŁAD 1: Malowanie linii ciągłej na nowej drodze

**Scenariusz**: Malujemy linię ciągłą 12cm na środku nowej drogi, odcinek 500m.

```
PRZYGOTOWANIE:
1. Uruchom system (zasilanie ON)
2. Poczekaj na self-test (LED zielony = OK)
3. Połącz telefon z WiFi "Trassar-Painter"
4. Otwórz http://192.168.4.1

KONFIGURACJA:
5. Wybierz wzorzec [P-2a] (Ciągła wąska 12cm)
6. Sprawdź czy selektor P3 = NORMALNY
7. Upewnij się że "Start od przerwy" jest ODZNACZONE

PRACA:
8. Ustaw maszynę na początku odcinka
9. Wciśnij [START/PAUZA]
10. Rusz do przodu (min. 3 km/h)
11. Gdy prędkość > 3 km/h → pistolet P2 włączy się automatycznie
12. Jedź równo, obserwuj panel (dystans, prędkość)
13. Na końcu odcinka wciśnij [STOP]

WYNIK:
- Raport zapisany automatycznie
- Powierzchnia: 500m × 0.12m = 60 m²
```

---

### PRZYKŁAD 2: Malowanie linii przerywanej P-1a

**Scenariusz**: Malujemy linię przerywaną długą (4m linia + 8m przerwa) na odcinku 1 km.

```
PRZYGOTOWANIE:
1-4. Jak w Przykładzie 1

KONFIGURACJA:
5. Wybierz wzorzec [P-1a] (Przerywana długa)
6. Selektor P3 = NORMALNY
7. "Start od przerwy" = ODZNACZONE (zaczynamy od linii)

PRACA:
8. Ustaw maszynę na początku
9. Wciśnij [START/PAUZA]
10. Rusz do przodu

OBSERWACJA CYKLOWANIA:
- 0-4m: Panel pokazuje "LINIA", pistolet P2 = ON
- 4-12m: Panel pokazuje "PRZERWA", pistolet P2 = OFF
- 12-16m: "LINIA", P2 = ON
- 16-24m: "PRZERWA", P2 = OFF
- ... i tak dalej automatycznie

11. Na końcu wciśnij [STOP]

WYNIK:
- Na 1000m: 1000 / 12 = 83 pełne cykle
- Namalowana długość linii: 83 × 4m = 332m
- Powierzchnia: 332m × 0.12m = 39.84 m²
```

---

### PRZYKŁAD 3: Dołączanie do istniejącej linii przerywanej (Start od przerwy)

**Scenariusz**: Kontynuujemy wczorajszą linię P-1b. Wczoraj skończyliśmy w połowie przerwy (zostało 2m przerwy).

```
KONFIGURACJA:
1. Wybierz wzorzec [P-1b] (2m linia + 4m przerwa)
2. Zaznacz checkbox ☑ "Start od przerwy"
3. Ustaw maszynę dokładnie 2m przed końcem przerwy
   (tam gdzie wczoraj skończyliśmy)

PRACA:
4. Wciśnij [START/PAUZA]
5. Rusz do przodu

OBSERWACJA:
- 0-4m: "PRZERWA" (pełna długość przerwy 4m, pistolety OFF)
  - Ale my zaczęliśmy 2m przed końcem, więc po 4m jesteśmy
    tam gdzie powinna zacząć się linia
- 4-6m: "LINIA", pistolet P2 = ON
- 6-10m: "PRZERWA", P2 = OFF
- ... cyklowanie automatyczne

UWAGA: Funkcja "Start od przerwy" startuje od PEŁNEJ przerwy wzorca.
Jeśli potrzebujesz precyzyjnego dołączenia - ustaw maszynę odpowiednio wcześniej.
```

---

### PRZYKŁAD 4: Malowanie linii podwójnej P-4 z selektorem P3

**Scenariusz**: Malujemy podwójną linię ciągłą. Najpierw w jedną stronę, potem wracamy drugą stroną drogi.

```
PRZEJAZD 1 (w stronę A→B):
1. Wybierz [P-4] (Podwójna ciągła)
2. Selektor P3 = NORMALNY
3. Ustaw maszynę po lewej stronie drogi
4. [START/PAUZA] → jedź
5. Pistolety P1 i P3 malują dwie linie równolegle
6. Na końcu [STOP]

PRZEJAZD 2 (powrót B→A):
7. Zawróć maszynę
8. Ustaw po prawej stronie drogi (malujemy po drugiej stronie)
9. Kliknij [SELEKTOR P3] → zmień na ODWRÓCONY
10. [START/PAUZA] → jedź z powrotem
11. P1 i P3 są zamienione, więc malują w tej samej orientacji
    względem drogi co w pierwszym przejeździe
12. [STOP]

WYNIK:
- Dwa przejazdy z odpowiednim ustawieniem selektora P3
- Linie równoległe po obu stronach drogi
```

---

### PRZYKŁAD 5: Pomiar odcinka przed malowaniem

**Scenariusz**: Chcemy sprawdzić długość odcinka do namalowania.

```
1. Panel WWW → [POMIAR]
2. System przechodzi w tryb MEASURING
3. Ustaw maszynę na początku
4. Jedź do końca odcinka (prędkość dowolna, pistolety OFF)
5. Odczytaj dystans z panelu: np. "Dystans: 847.3 m"
6. [STOP] → powrót do IDLE

PLANOWANIE:
- Wiemy że odcinek ma 847m
- Dla P-1a (4m+8m) to 847/12 = 70 cykli
- Namalujemy 70 × 4m = 280m linii
- Zużyjemy farbę na ~280m × 0.12m = 33.6 m²
```

---

### PRZYKŁAD 6: Test pistoletów w trybie serwisowym

**Scenariusz**: Przed rozpoczęciem pracy chcemy sprawdzić czy wszystkie pistolety działają.

```
1. Panel WWW → [TRYB SERWISOWY]
2. Ustaw maszynę nad miejscem gdzie można nanieść testową farbę

TEST KAŻDEGO PISTOLETU:
3. Przytrzymaj [P-2a] → P2 strzela (12cm na osi)
4. Puść → P2 przestaje
5. Przytrzymaj [P-2b] → P4 strzela (24cm na osi)
6. Puść → P4 przestaje
7. Przytrzymaj [P-4] → P1 i P3 strzelają (podwójna)
8. Puść → P1 i P3 przestają
9. Przytrzymaj [P-7b] → P6 strzela (krawędź szeroka)
10. Puść → P6 przestaje
11. Przytrzymaj [P-7d] → P5 strzela (krawędź wąska)
12. Puść → P5 przestaje

13. [WYJDŹ] → powrót do IDLE

WYNIK:
- Sprawdzone wszystkie 6 pistoletów
- Jeśli któryś nie działa → sprawdź przekaźnik, dyszę, farbę
```

---

### PRZYKŁAD 7: Praca przez cały dzień z przerwami

**Scenariusz**: 8-godzinny dzień pracy z przerwami na obiad i tankowanie.

```
08:00 - START DNIA:
1. Włącz system, poczekaj na self-test
2. Sprawdź kartę SD (panel → info o karcie)
3. Wybierz wzorzec na pierwszy odcinek

08:15 - PIERWSZY ODCINEK:
4. [START/PAUZA] → malowanie
5. Po 2 godzinach → [START/PAUZA] = PAUZA
6. Tankowanie farby (system w pauzie, raport się nie kończy)
7. [START/PAUZA] → wznowienie
8. Kontynuacja malowania

12:00 - PRZERWA OBIADOWA:
9. [STOP] → zakończenie pierwszego raportu
10. Raport #1 zapisany na kartę SD
11. Wyłączenie systemu (opcjonalnie)

13:00 - DRUGI ODCINEK:
12. Włączenie systemu (jeśli był wyłączony)
13. Wybór wzorca
14. [START/PAUZA] → nowy raport #2 zaczyna się automatycznie
15. Malowanie przez popołudnie

16:30 - KONIEC DNIA:
16. [STOP] → zakończenie raportu #2
17. Panel → Raporty → sprawdź oba raporty
18. Eksport CSV obu raportów (opcjonalnie)
19. Wyłączenie systemu

PODSUMOWANIE:
- Raport #1: 08:15-12:00 (3h 45min), np. 2500 m², wzorce P-2a + P-1a
- Raport #2: 13:00-16:30 (3h 30min), np. 2100 m², wzorce P-2b + P-4
- Łącznie: 4600 m² w ciągu dnia
```

---

### PRZYKŁAD 8: Obsługa sytuacji awaryjnej (E-STOP)

**Scenariusz**: Podczas malowania zauważasz przeszkodę na drodze.

```
SYTUACJA:
1. Malujesz w trybie WORKING
2. Zauważasz pieszego wchodzącego na jezdnię
3. NATYCHMIAST wciskasz E-STOP (czerwony grzybek)

REAKCJA SYSTEMU:
- Wszystkie pistolety NATYCHMIAST wyłączone
- LED czerwony świeci stale
- 5 sygnałów dźwiękowych
- Panel pokazuje: "TRYB AWARYJNY - E-STOP AKTYWNY"

PO USUNIĘCIU ZAGROŻENIA:
4. Upewnij się że droga jest wolna
5. Odblokuj E-STOP (przekręć grzybek)
6. Panel WWW → [RESETUJ E-STOP]
7. Podaj hasło: admin / trassar251
8. System wraca do IDLE
9. Możesz wznowić pracę: [START/PAUZA]

UWAGA:
- Raport NIE został przerwany (pauza automatyczna)
- Po resecie możesz kontynuować ten sam raport
- Jeśli chcesz nowy raport → [STOP] przed [START/PAUZA]
```

---

## 17. ROZWIĄZYWANIE PROBLEMÓW

### 17.1 Tabela problemów

| Problem | Przyczyna | Rozwiązanie |
|---------|-----------|-------------|
| LED czerwony po starcie | E-STOP wciśnięty | Zwolnij grzybek, resetuj w panelu |
| Brak malowania mimo WORKING | Prędkość < 3 km/h | Jedź szybciej (min. 3 km/h) |
| Brak malowania mimo WORKING | Faza "przerwa" w cyklu | Poczekaj na fazę "linia" lub jedź dalej |
| Brak dystansu | Enkoder niepodłączony | Sprawdź GPIO 8,9,10 |
| Karta SD nie wykryta | Karta uszkodzona/nieprawidłowa | Użyj karty FAT32, max 32GB |
| Panel WWW nie działa | WiFi AP nie wystartował | Restart ESP32 |
| Automatyczna pauza | Deadman timeout (30s) | Wciśnij przycisk co <30 sekund |
| Pistolety nie włączają się | Przekaźnik uszkodzony | Test w trybie serwisowym |
| Błędny dystans | Kalibracja nieprawidłowa | Przeprowadź kalibrację enkodera |
| System nie reaguje | Watchdog zadziałał | Poczekaj na automatyczny restart |

### 17.2 Kody błędów

| Kod | Typ błędu | Opis | Rozwiązanie |
|-----|-----------|------|-------------|
| 1 | ERR_ESTOP_PRESSED | E-STOP wciśnięty | Zwolnij E-STOP, resetuj w panelu |
| 2 | ERR_WATCHDOG_TIMEOUT | System zawieszony | Automatyczny restart |
| 3 | ERR_ENCODER_STALL | Enkoder zatrzymany | Sprawdź mechanizm pomiarowy |
| 4 | ERR_ENCODER_DISCONNECTED | Enkoder odłączony | Sprawdź okablowanie |
| 5 | ERR_SPEED_INVALID | Prędkość poza zakresem | Sprawdź enkoder |
| 6 | ERR_GUN_FEEDBACK | Błąd przekaźnika | Sprawdź przekaźnik |
| 7 | ERR_RTC_FAILED | Zegar RTC nie działa | Sprawdź baterię RTC |
| 8 | ERR_FILESYSTEM_FULL | Brak miejsca na raporty | Usuń stare raporty |
| 9 | ERR_CALIBRATION_DRIFT | Kalibracja dryfuje | Przeprowadź kalibrację |
| 10 | ERR_HEARTBEAT_TIMEOUT | System nie odpowiada | Restart systemu |
| 11 | ERR_SELF_TEST_FAILED | Self-test nie przeszedł | Sprawdź komponenty |
| 12 | ERR_SPEED_TOO_LOW | Prędkość < 3 km/h | Jedź szybciej |
| 13 | ERR_DEADMAN_TIMEOUT | Brak potwierdzenia operatora | Wciśnij przycisk |
| 14 | ERR_SD_CARD_FAILED | Karta SD nie działa | Sprawdź kartę |

---

## 18. DANE TECHNICZNE

### 18.1 Parametry systemu

| Parametr | Wartość |
|----------|---------|
| Mikrokontroler | ESP32-S3 DevKitC-1 |
| Flash | 16 MB |
| PSRAM | 8 MB |
| Firmware | v7.0.0 SD-SPEED EDITION |
| Wzorce malowania | 15 (P-1a do P-7d) |
| Kanały pistoletów | 6 (P1-P6) |
| Minimalna prędkość malowania | 3 km/h |
| Wyświetlacz | TFT ILI9341 2.8" (320×240) |
| Karta pamięci | microSD FAT32 (max 32GB) |
| WiFi AP | Trassar-Painter / 12345678 |
| Panel WWW | http://192.168.4.1 |
| Hasło OTA | trassar2024 |
| Hasło admin | trassar251 |
| Zasilanie | 5V / 2A minimum |

### 18.2 Pinout GPIO (skrót)

| Funkcja | GPIO |
|---------|------|
| Przekaźniki P1-P6 | 21, 47, 48, 19, 38, 39 |
| START/PAUZA | 40 |
| STOP | 41 |
| E-STOP | 42 |
| Enkoder CLK/DT/SW | 8, 9, 10 |
| TFT (CS/DC/RST/BL) | 14, 15, 16, 17 |
| Karta SD CS | 2 |
| RTC I2C (SDA/SCL) | 7, 18 |
| LEDy (G/R/Y) | 35, 36, 37 |
| Buzzer | 46 |
| Joystick (X/Y/SW) | 4, 5, 6 |
| Selektor P3 | 20 |

Pełny schemat połączeń: [SCHEMAT_POLACZEN.md](SCHEMAT_POLACZEN.md)

---

*Trassar-Painter v7.0.0 SD-SPEED EDITION*
*Autor: Trassar251 | Data: 2026-02-03*
*Dokument wygenerowany automatycznie*
