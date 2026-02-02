/**
 * @file pins.h
 * @brief JEDYNE ŹRÓDŁO PRAWDY o pinach GPIO - Trassar-Painter v6.0.0
 *
 * ═══════════════════════════════════════════════════════════════════
 * WSZYSTKIE PRZYPISANIA PINÓW GPIO ESP32-S3 SĄ ZDEFINIOWANE TUTAJ
 * Żaden inny plik NIE MOŻE definiować pinów GPIO!
 * ═══════════════════════════════════════════════════════════════════
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
#define RELAY_4     45  // P4 (24cm) - oś jezdni szeroki
#define RELAY_5     38  // P5 (12cm) - krawędziowy
#define RELAY_6     39  // P6 (24cm) - krawędziowy szeroki

// Tablica pinów przekaźników - UŻYWAJ ZAWSZE ZAMIAST RELAY_1+i !!!
// Rozwiązuje krytyczny bug z v5.3.0 (E-STOP wyłączał złe piny)
static const int RELAY_PINS[6] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4, RELAY_5, RELAY_6};
static const int RELAY_COUNT = 6;

// ============================================================================
// PRZYCISKI STERUJĄCE
// ============================================================================
// Aktywne LOW z wewnętrznym PULLUP
#define BTN_START   40  // START malowania/pomiaru
#define BTN_STOP    41  // STOP
#define BTN_PAUSE   19  // PAUSE/RESUME

// ============================================================================
// SYSTEM BEZPIECZEŃSTWA (SAFETY)
// ============================================================================
#define BTN_EMERGENCY_STOP  42  // E-STOP - czerwony przycisk awaryjny (aktywny LOW)
#define BUZZER_PIN          46  // Buzzer - alerty dźwiękowe
#define LED_STATUS_GREEN    35  // LED zielony - system OK
#define LED_STATUS_RED      36  // LED czerwony - błąd/emergency
#define LED_STATUS_YELLOW   37  // LED żółty - ostrzeżenie

// ============================================================================
// SELEKTOR P3 (przełącznik fizyczny)
// ============================================================================
// LOW = normalne przypisanie pistoletów
// HIGH = odwrócone (zamiana P1 <-> P3)
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
// TFT ILI9341 - piny zdefiniowane w platformio.ini build_flags
// ============================================================================
// MOSI = 11 (SPI)
// MISO = 13 (SPI)
// SCK  = 12 (SPI Clock)
// CS   = 14 (Chip Select)
// DC   = 15 (Data/Command)
// RST  = 16 (Reset)
// LED  = 17 (Podświetlenie)
//
// UWAGA: Te piny są konfigurowane przez -D flagi w platformio.ini
// i nie powinny być zmieniane tutaj!

#endif // PINS_H
