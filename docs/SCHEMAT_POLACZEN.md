# SCHEMAT POŁĄCZEŃ ELEKTRYCZNYCH

## Trassar-Painter v7.0.0 - Komputer Malowarki Drogowej

> **ŹRÓDŁO PRAWDY**: Wszystkie piny GPIO zdefiniowane w `src/pins.h`

---

## PLATFORMA

| Parametr | Wartość |
|----------|---------|
| Mikrokontroler | ESP32-S3 DevKitC-1 |
| Flash | 16MB (DIO) |
| PSRAM | 8MB (OPI) |
| Napięcie logiki | 3.3V |
| Zasilanie | 5V USB lub VIN |

---

## TABELA POŁĄCZEŃ GPIO

### Przekaźniki pistoletów (OUTPUT, aktywne HIGH)

| Pin GPIO | Oznaczenie | Funkcja | Pistolet | Szerokość |
|----------|-----------|---------|----------|-----------|
| 21 | RELAY_1 | Przekaźnik P1 | Pistolet 1 | 12cm (oś) |
| 47 | RELAY_2 | Przekaźnik P2 | Pistolet 2 | 12cm (oś) |
| 48 | RELAY_3 | Przekaźnik P3 | Pistolet 3 | 12cm (oś) |
| **19** | **RELAY_4** | **Przekaźnik P4** | **Pistolet 4** | **24cm (oś)** |
| 38 | RELAY_5 | Przekaźnik P5 | Pistolet 5 | 12cm (krawędź) |
| 39 | RELAY_6 | Przekaźnik P6 | Pistolet 6 | 24cm (krawędź) |

> **UWAGA v7.0.0**: RELAY_4 przeniesiony z GPIO 45 (strapping pin!) na GPIO 19 (zwolniony po usunięciu BTN_PAUSE).

**Schemat podłączenia przekaźnika:**
```
ESP32 GPIO ──> Rezystor 1kΩ ──> Baza NPN (np. BC547)
                                 Kolektor ──> Cewka przekaźnika ──> +5V
                                 Emiter ──> GND
                                 Dioda 1N4007 równolegle z cewką (katoda na +5V)
```

### Przyciski sterujące (INPUT_PULLUP, aktywne LOW)

| Pin GPIO | Oznaczenie | Funkcja | Typ przerwania |
|----------|-----------|---------|----------------|
| **40** | **BTN_START_PAUSE** | **Start / Pauza (jeden przycisk!)** | FALLING |
| 41 | BTN_STOP | Stop | FALLING |

> **UWAGA v7.0.0**: Osobny przycisk PAUSE (GPIO 19) usunięty. Jeden przycisk START/PAUZA na GPIO 40 przełącza tryby: IDLE→WORKING, WORKING→PAUSED, PAUSED→WORKING.

**Schemat podłączenia przycisku:**
```
ESP32 GPIO ──┬── Przycisk ──> GND
             │
             └── (wewnętrzny PULLUP do 3.3V)
```

### System bezpieczeństwa (SAFETY)

| Pin GPIO | Oznaczenie | Funkcja | Kierunek |
|----------|-----------|---------|----------|
| 42 | BTN_EMERGENCY_STOP | E-STOP (czerwony grzybek) | INPUT_PULLUP, **CHANGE** |
| 46 | BUZZER_PIN | Buzzer alarmowy | OUTPUT |
| 35 | LED_STATUS_GREEN | LED zielony (system OK) | OUTPUT |
| 36 | LED_STATUS_RED | LED czerwony (błąd/awaria) | OUTPUT |
| 37 | LED_STATUS_YELLOW | LED żółty (ostrzeżenie) | OUTPUT |

**E-STOP** musi być przyciskiem NC (Normally Closed) z mechanizmem blokady:
```
ESP32 GPIO 42 ──┬── Przycisk E-STOP (NC) ──> GND
                │
                └── (wewnętrzny PULLUP)

Stan normalny: GPIO = LOW (NC = zamknięty = GND)
Wciśnięty: GPIO = HIGH (obwód otwarty = PULLUP)

v7.0.0: ISR na CHANGE + digitalRead==HIGH (poprawna logika NC)
```

### Karta SD (v7.0.0 - współdzielona magistrala SPI z TFT)

| Pin GPIO | Oznaczenie | Funkcja |
|----------|-----------|---------|
| **2** | **SD_CS_PIN** | **Chip Select karty SD** |
| 11 | (TFT_MOSI) | Współdzielony MOSI |
| 13 | (TFT_MISO) | Współdzielony MISO |
| 12 | (TFT_SCLK) | Współdzielony SCK |

```
Czytnik kart SD (zintegrowany z modułem TFT):
  CS   ──> GPIO 2
  MOSI ──> GPIO 11 (współdzielony z TFT)
  MISO ──> GPIO 13 (współdzielony z TFT)
  SCK  ──> GPIO 12 (współdzielony z TFT)
  VCC  ──> 3.3V
  GND  ──> GND
```

### Selektor P3 (przełącznik dwupozycyjny)

| Pin GPIO | Oznaczenie | Funkcja |
|----------|-----------|---------|
| 20 | SEL_P3 | Selektor P3 (LOW=normalne, HIGH=odwrócone) |

### Joystick analogowy (2 osie + przycisk)

| Pin GPIO | Oznaczenie | Funkcja | Typ |
|----------|-----------|---------|-----|
| 4 | JOY_VRX | Oś X (lewo/prawo) | ADC1_CH3 (0-4095) |
| 5 | JOY_VRY | Oś Y (góra/dół) | ADC1_CH4 (0-4095) |
| 6 | JOY_SW | Przycisk | INPUT_PULLUP |

### Enkoder pomiarowy (kwadraturowy)

| Pin GPIO | Oznaczenie | Funkcja | Przerwanie |
|----------|-----------|---------|------------|
| 8 | ENC_CLK | Impulsy pomiaru | RISING |
| 9 | ENC_DT | Kierunek (opcjonalnie) | - |
| 10 | ENC_SW | Przycisk enkodera | FALLING |

### RTC DS1307 (I2C)

| Pin GPIO | Oznaczenie | Funkcja |
|----------|-----------|---------|
| 7 | RTC_SDA | I2C Data |
| 18 | RTC_SCL | I2C Clock |

### Wyświetlacz TFT ILI9341 2.8" (SPI)

| Pin GPIO | Oznaczenie | Funkcja |
|----------|-----------|---------|
| 11 | TFT_MOSI | SPI Master Out Slave In |
| 13 | TFT_MISO | SPI Master In Slave Out |
| 12 | TFT_SCLK | SPI Clock |
| 14 | TFT_CS | Chip Select |
| 15 | TFT_DC | Data/Command |
| 16 | TFT_RST | Reset |
| 17 | TFT_BL | Backlight (podświetlenie) |

> **UWAGA**: Piny TFT są konfigurowane w `platformio.ini` przez flagi `-DTFT_xxx`.

---

## PODSUMOWANIE UŻYCIA GPIO

| GPIO | Funkcja | Kierunek |
|------|---------|----------|
| 2 | SD_CS | OUTPUT |
| 4 | JOY_VRX | ADC INPUT |
| 5 | JOY_VRY | ADC INPUT |
| 6 | JOY_SW | INPUT_PULLUP |
| 7 | RTC_SDA | I2C |
| 8 | ENC_CLK | INPUT_PULLUP (ISR) |
| 9 | ENC_DT | INPUT_PULLUP |
| 10 | ENC_SW | INPUT_PULLUP (ISR) |
| 11 | TFT_MOSI / SD_MOSI | SPI OUTPUT |
| 12 | TFT_SCLK / SD_SCK | SPI OUTPUT |
| 13 | TFT_MISO / SD_MISO | SPI INPUT |
| 14 | TFT_CS | OUTPUT |
| 15 | TFT_DC | OUTPUT |
| 16 | TFT_RST | OUTPUT |
| 17 | TFT_BL | OUTPUT |
| 18 | RTC_SCL | I2C |
| 19 | RELAY_4 | OUTPUT |
| 20 | SEL_P3 | INPUT_PULLUP |
| 21 | RELAY_1 | OUTPUT |
| 35 | LED_GREEN | OUTPUT |
| 36 | LED_RED | OUTPUT |
| 37 | LED_YELLOW | OUTPUT |
| 38 | RELAY_5 | OUTPUT |
| 39 | RELAY_6 | OUTPUT |
| 40 | BTN_START_PAUSE | INPUT_PULLUP (ISR) |
| 41 | BTN_STOP | INPUT_PULLUP (ISR) |
| 42 | E-STOP | INPUT_PULLUP (ISR CHANGE) |
| 46 | BUZZER | OUTPUT |
| 47 | RELAY_2 | OUTPUT |
| 48 | RELAY_3 | OUTPUT |

**Łącznie: 28 pinów GPIO wykorzystanych** (zamiana: GPIO 45 zwolniony, GPIO 2 dodany)

---

## ZASILANIE

| Komponent | Napięcie | Prąd (typowy) |
|-----------|----------|---------------|
| ESP32-S3 | 5V (USB/VIN) | 300mA |
| TFT ILI9341 + SD | 3.3V | 100mA |
| RTC DS1307 | 5V | 5mA |
| 6x Przekaźnik | 5V (cewka) | 6 × 70mA = 420mA |
| LEDy (3x) | 3.3V | 3 × 20mA = 60mA |
| Buzzer | 3.3V | 30mA |
| Enkoder | 3.3V/5V | 20mA |
| Joystick | 3.3V | 10mA |
| **SUMA** | | **~945mA (5V)** |

> **ZALECENIE**: Zasilacz 5V / 2A minimum.
