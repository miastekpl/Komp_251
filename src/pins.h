/**
 * @file pins.h
 * @brief JEDYNE ŹRÓDŁO PRAWDY o pinach GPIO - Trassar-Painter v7.0.0
 *
 * ═══════════════════════════════════════════════════════════════════
 * WSZYSTKIE PRZYPISANIA PINÓW GPIO ESP32-S3 SĄ ZDEFINIOWANE TUTAJ
 * Żaden inny plik NIE MOŻE definiować pinów GPIO!
 * ═══════════════════════════════════════════════════════════════════
 *
 * ZMIANY v7.0.0:
 * - RELAY_4 przeniesiony z GPIO 45 (strapping!) na GPIO 19
 * - BTN_PAUSE usunięty - jeden przycisk START/PAUZA na GPIO 40
 * - Dodany SD_CS_PIN na GPIO 2 (karta SD zintegrowana z TFT)
 * - RELAY_PINS/RELAY_COUNT: extern const (definicja w pins.cpp)
 *
 * Platforma: ESP32-S3 DevKitC-1 (16MB Flash, 8MB PSRAM)
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef PINS_H
#define PINS_H

// ============================================================================
// PRZEKAŹNIKI PISTOLETÓW (6 kanałów)
// ============================================================================
// Aktywne HIGH - podanie HIGH włącza pistolet
#define RELAY_1     21  // P1 (12cm) - oś jezdni
#define RELAY_2     47  // P2 (12cm) - oś jezdni
#define RELAY_3     48  // P3 (12cm) - oś jezdni
#define RELAY_4     19  // P4 (24cm) - PRZENIESIONY z GPIO 45 (strapping pin!)
#define RELAY_5     38  // P5 (12cm) - krawędziowy
#define RELAY_6     39  // P6 (24cm) - krawędziowy szeroki

// Tablica pinów przekaźników - definicja w pins.cpp
// UŻYWAJ ZAWSZE ZAMIAST RELAY_1+i !!!
extern const int RELAY_PINS[6];
extern const int RELAY_COUNT;

// ============================================================================
// PRZYCISKI STERUJĄCE
// ============================================================================
// v7.0.0: JEDEN przycisk START/PAUZA zamiast osobnych START + PAUSE
// Aktywne LOW z wewnętrznym PULLUP
#define BTN_START_PAUSE  40  // START / PAUZA (jeden przycisk!)
#define BTN_STOP         41  // STOP

// ============================================================================
// SYSTEM BEZPIECZEŃSTWA (SAFETY)
// ============================================================================
#define BTN_EMERGENCY_STOP  42  // E-STOP - przycisk awaryjny NC (Normally Closed)
#define BUZZER_PIN          46  // Buzzer - alerty dźwiękowe (strapping, OK po boot)
#define LED_STATUS_GREEN    35  // LED zielony - system OK
#define LED_STATUS_RED      36  // LED czerwony - błąd/emergency
#define LED_STATUS_YELLOW   37  // LED żółty - ostrzeżenie

// ============================================================================
// SELEKTOR P3 (przełącznik fizyczny)
// ============================================================================
#define SEL_P3      20

// ============================================================================
// JOYSTICK ANALOGOWY (nawigacja menu)
// ============================================================================
#define JOY_VRX     4   // ADC1_CH3 - Oś X (lewo/prawo) 0-4095
#define JOY_VRY     5   // ADC1_CH4 - Oś Y (góra/dół) 0-4095
#define JOY_SW      6   // Przycisk joysticka (aktywny LOW)

// ============================================================================
// ENKODER POMIAROWY (mierzenie dystansu i prędkości)
// ============================================================================
#define ENC_CLK     8   // Impulsy pomiaru (RISING edge)
#define ENC_DT      9   // Kierunek (opcjonalnie)
#define ENC_SW      10  // Przycisk enkodera (zatwierdzanie w MENU)

// ============================================================================
// RTC DS1307 (I2C)
// ============================================================================
#define RTC_SDA     7   // I2C Data
#define RTC_SCL     18  // I2C Clock

// ============================================================================
// KARTA SD (zintegrowana z wyświetlaczem TFT) - NOWE v7.0.0
// ============================================================================
#define SD_CS_PIN   2   // Chip Select karty SD

// ============================================================================
// TFT ILI9341 - piny zdefiniowane w platformio.ini build_flags
// ============================================================================
// MOSI = 11 (SPI - współdzielone z SD)
// MISO = 13 (SPI - współdzielone z SD)
// SCK  = 12 (SPI Clock - współdzielone z SD)
// CS   = 14 (Chip Select TFT)
// DC   = 15 (Data/Command)
// RST  = 16 (Reset)
// LED  = 17 (Podświetlenie)

#endif // PINS_H
