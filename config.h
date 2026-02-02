/**
 * @file config.h
 * @brief Konfiguracja systemu malowania pasów drogowych
 * @author Trassar251 - Professional Road Marking System
 * @version 1.0.0
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// DEFINICJE PINÓW - ESP32-S3
// ============================================================================

// Enkoder KY-040
#define ENCODER_CLK   4   // Pin CLK enkodera
#define ENCODER_DT    5   // Pin DT enkodera
#define ENCODER_SW    6   // Przycisk enkodera (selektor)

// Przyciski sterujące
#define BTN_START     14  // Przycisk START/PAUZA
#define BTN_STOP      15  // Przycisk STOP

// Przyciski wyboru wzorców (12 wzorców)
#define BTN_P1A       16
#define BTN_P1B       17
#define BTN_P1C       18
#define BTN_P1D       21
#define BTN_P1E       47
#define BTN_P2A       48
#define BTN_P2B       45
#define BTN_P3A       35
#define BTN_P3B       36
#define BTN_P4        37
#define BTN_P6        38
#define BTN_P7A       39
#define BTN_P7B       40
#define BTN_P7C       41
#define BTN_P7D       42

// Przełącznik odwracania P-3a/P-3b
#define SWITCH_REVERSE 46  // Przełącznik LEFT/RIGHT dla P-3a i P-3b

// Przekaźniki pistoletów malarskich (6 pistoletów)
#define RELAY_GUN_1   1   // Pistolet 1 (lewy zewnętrzny)
#define RELAY_GUN_2   2   // Pistolet 2
#define RELAY_GUN_3   3   // Pistolet 3 (lewy wewnętrzny)
#define RELAY_GUN_4   33  // Pistolet 4 (prawy wewnętrzny)
#define RELAY_GUN_5   34  // Pistolet 5
#define RELAY_GUN_6   0   // Pistolet 6 (prawy zewnętrzny)

// ============================================================================
// PARAMETRY ENKODERA
// ============================================================================

#define ENCODER_PULSES_PER_REV  20      // Impulsy na obrót KY-040
#define CALIBRATION_DISTANCE    1000.0  // Dystans kalibracji w cm (10m)
#define DEFAULT_PULSES_PER_CM   2.0     // Domyślna wartość (zostanie skalibrowana)

// ============================================================================
// PARAMETRY WYŚWIETLACZA
// ============================================================================

#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  240
#define SCREEN_ROTATION 1  // Landscape

// Kolory
#define COLOR_BACKGROUND  0x0000  // Czarny
#define COLOR_TEXT_WHITE  0xFFFF  // Biały
#define COLOR_TEXT_YELLOW 0xFFE0  // Żółty
#define COLOR_TEXT_GREEN  0x07E0  // Zielony
#define COLOR_TEXT_RED    0xF800  // Czerwony
#define COLOR_TEXT_CYAN   0x07FF  // Cyjan
#define COLOR_HIGHLIGHT   0x051D  // Niebieski ciemny
#define COLOR_MENU_BG     0x18E3  // Szary

// ============================================================================
// PARAMETRY CZASOWE
// ============================================================================

#define DEBOUNCE_DELAY       50    // ms - odstęp debouncingu przycisków
#define LONG_PRESS_STOP      1000  // ms - długie naciśnięcie STOP (menu)
#define LONG_PRESS_SELECT    500   // ms - długie naciśnięcie selektora (wejście)
#define EXIT_LONG_PRESS      2000  // ms - długie naciśnięcie STOP (wyjście z funkcji)
#define SCREEN_REFRESH_MS    100   // ms - odświeżanie ekranu pracy
#define SPEED_UPDATE_MS      200   // ms - aktualizacja prędkości

// ============================================================================
// PARAMETRY BEZPIECZEŃSTWA
// ============================================================================

#define MIN_SPEED_CMPS       56.0  // Minimalna prędkość dla pistoletów (2 km/h = 56 cm/s)
#define MIN_SPEED_KMPH       2.0   // Minimalna prędkość w km/h
#define WATCHDOG_TIMEOUT_S   10    // Watchdog timeout w sekundach

// ============================================================================
// KONFIGURACJA WIFI - Access Point Mode
// ============================================================================

#define WIFI_ENABLED         true
#define WIFI_AP_SSID         "Trassar-Painter"
#define WIFI_AP_PASSWORD     ""                  // Pusty = otwarty AP
#define WIFI_AP_CHANNEL      6
#define WIFI_AP_HIDDEN       false
#define WIFI_AP_MAX_CONN     4
#define WIFI_HOSTNAME        "trassar251"
#define WIFI_PORT            80

// Ostrzeżenie kompilacji
#if defined(WIFI_ENABLED) && WIFI_ENABLED == true
    #warning "WiFi ENABLED with TEST credentials! Change before production!"
#endif

// ============================================================================
// PARAMETRY RTOS
// ============================================================================

#define TASK_STACK_SIZE_SMALL    2048
#define TASK_STACK_SIZE_MEDIUM   4096
#define TASK_STACK_SIZE_LARGE    8192

// Priorytety tasków (wyższe = ważniejsze)
#define PRIORITY_CRITICAL    5  // Encoder ISR, Safety
#define PRIORITY_HIGH        4  // Painting control
#define PRIORITY_MEDIUM      3  // Display, Buttons
#define PRIORITY_LOW         2  // WiFi, Logging
#define PRIORITY_IDLE        1  // Stats, Housekeeping

// Core assignment
#define CORE_CONTROL         1  // Core 1: Critical control (encoder, painting, safety)
#define CORE_UI              0  // Core 0: UI (display, WiFi, buttons)

// ============================================================================
// WZORCE MALOWANIA
// ============================================================================

enum PatternType {
    PATTERN_NONE = 0,
    PATTERN_P1A,   // Przerywana długa 4m/8m 12cm
    PATTERN_P1B,   // Przerywana krótka 2m/4m 12cm
    PATTERN_P1C,   // Wydzielająca 2m/2m 12cm
    PATTERN_P1D,   // Prowadząca wąska 1m/1m 12cm
    PATTERN_P1E,   // Prowadząca szeroka 1m/1m 24cm
    PATTERN_P2A,   // Ciągła wąska 12cm
    PATTERN_P2B,   // Ciągła szeroka 24cm
    PATTERN_P3A,   // Przekraczalna długa 4m/2m 12cm (podwójna)
    PATTERN_P3B,   // Przekraczalna krótka 1m/1m 12cm (podwójna)
    PATTERN_P4,    // Podwójna ciągła 24cm
    PATTERN_P6,    // Ostrzegawcza 4m/2m 12cm
    PATTERN_P7A,   // Krawędziowa przerywana szeroka 1m/1m 24cm
    PATTERN_P7B,   // Krawędziowa ciągła szeroka 24cm
    PATTERN_P7C,   // Krawędziowa przerywana wąska 1m/1m 12cm
    PATTERN_P7D    // Krawędziowa ciągła wąska 12cm
};

// Struktura wzorca
struct Pattern {
    const char* name;          // Nazwa wzorca
    float lineLength;          // Długość linii w cm (0 = ciągła)
    float gapLength;           // Długość przerwy w cm
    uint8_t width;             // Szerokość w cm (12 lub 24)
    bool isDouble;             // Czy podwójna linia
    bool isContinuous;         // Czy ciągła
    uint8_t guns;              // Maska pistoletów (bit 0-5)
};

// Definicje wzorców
// Pistolety: P1,P2,P3,P4 = oś jezdni | P5,P6 = krawędź
// Szerokości: P1,P2,P3,P5=12cm | P4,P6=24cm
// Maska: bit0=P1, bit1=P2, bit2=P3, bit3=P4, bit4=P5, bit5=P6
const Pattern PATTERNS[] = {
    {"NONE",  0,    0,    0,  false, false, 0b000000}, // PATTERN_NONE
    {"P-1a",  400,  800,  12, false, false, 0b000010}, // PATTERN_P1A  → P2 (12cm)
    {"P-1b",  200,  400,  12, false, false, 0b000010}, // PATTERN_P1B  → P2 (12cm)
    {"P-1c",  200,  200,  12, false, false, 0b000010}, // PATTERN_P1C  → P2 (12cm)
    {"P-1d",  100,  100,  12, false, false, 0b000010}, // PATTERN_P1D  → P2 (12cm)
    {"P-1e",  100,  100,  24, false, false, 0b001000}, // PATTERN_P1E  → P4 (24cm)
    {"P-2a",  0,    0,    12, false, true,  0b000010}, // PATTERN_P2A  → P2 (12cm)
    {"P-2b",  0,    0,    24, false, true,  0b001000}, // PATTERN_P2B  → P4 (24cm)
    {"P-3a",  400,  200,  24, true,  false, 0b000101}, // PATTERN_P3A  → P1+P3 (2x12=24cm)
    {"P-3b",  100,  100,  24, true,  false, 0b000101}, // PATTERN_P3B  → P1+P3 (2x12=24cm)
    {"P-4",   0,    0,    24, true,  true,  0b000101}, // PATTERN_P4   → P1+P3 (2x12=24cm)
    {"P-6",   400,  200,  12, false, false, 0b010000}, // PATTERN_P6   → P5 (12cm)
    {"P-7a",  100,  100,  24, false, false, 0b100000}, // PATTERN_P7A  → P6 (24cm)
    {"P-7b",  0,    0,    24, false, true,  0b100000}, // PATTERN_P7B  → P6 (24cm)
    {"P-7c",  100,  100,  12, false, false, 0b010000}, // PATTERN_P7C  → P5 (12cm)
    {"P-7d",  0,    0,    12, false, true,  0b010000}  // PATTERN_P7D  → P5 (12cm)
};

// ============================================================================
// STANY SYSTEMU
// ============================================================================

enum SystemState {
    STATE_IDLE,           // Ekran pracy - gotowy
    STATE_WORKING,        // Malowanie w toku
    STATE_PAUSED,         // Malowanie wstrzymane
    STATE_MENU,           // Menu główne
    STATE_CALIBRATION,    // Tryb kalibracji
    STATE_DISTANCE_MEASURE // Tryb pomiaru dystansu
};

// Pozycje menu
enum MenuOption {
    MENU_CALIBRATION = 0,
    MENU_DISTANCE_MEASURE,
    MENU_RESET_STATS,
    MENU_SETTINGS,
    MENU_EXIT,
    MENU_OPTIONS_COUNT
};

// ============================================================================
// MAKRA POMOCNICZE
// ============================================================================

// Konwersje jednostek
#define CM_TO_M(cm)     ((cm) / 100.0f)
#define M_TO_CM(m)      ((m) * 100.0f)
#define CM_TO_KM(cm)    ((cm) / 100000.0f)
#define CMPS_TO_KMPH(cmps) ((cmps) * 0.036f) // cm/s to km/h

// Obliczanie m² (dystans_cm * szerokość_cm / 10000)
#define CALC_M2(dist_cm, width_cm) ((dist_cm) * (width_cm) / 10000.0f)

#endif // CONFIG_H
