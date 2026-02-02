/**
 * @file types.h
 * @brief Definicje typów, struktur i enumeracji - Trassar-Painter v6.0.0
 *
 * Centralne miejsce definicji wszystkich typów danych używanych w systemie.
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef TYPES_H
#define TYPES_H

#include "config.h"

// ============================================================================
// TRYBY PRACY SYSTEMU
// ============================================================================
enum SystemMode {
    MODE_IDLE,          // Bezczynny - czeka na komendę
    MODE_WORKING,       // Malowanie aktywne
    MODE_PAUSED,        // Pauza (wzorzec zachowany)
    MODE_MEASURING,     // Pomiar dystansu (bez malowania)
    MODE_MENU,          // Menu konfiguracji (nawigacja joystickiem)
    MODE_SERVICE,       // Tryb serwisowy - test pistoletów
    MODE_CALIBRATING,   // Tryb kalibracji automatycznej (jedź 10m)
    MODE_EMERGENCY      // Tryb awaryjny - E-STOP aktywny
};

// ============================================================================
// TYPY BŁĘDÓW BEZPIECZEŃSTWA
// ============================================================================
enum ErrorType {
    ERR_NONE = 0,
    ERR_ESTOP_PRESSED,          // E-STOP wciśnięty
    ERR_WATCHDOG_TIMEOUT,       // Watchdog timeout
    ERR_ENCODER_STALL,          // Enkoder zatrzymany podczas pracy
    ERR_ENCODER_DISCONNECTED,   // Enkoder odłączony (brak impulsów)
    ERR_SPEED_INVALID,          // Prędkość nierealistyczna
    ERR_GUN_FEEDBACK,           // Błąd przekaźnika pistoletu
    ERR_RTC_FAILED,             // RTC nie działa
    ERR_FILESYSTEM_FULL,        // LittleFS pełny
    ERR_CALIBRATION_DRIFT,      // Kalibracja dryftuje
    ERR_HEARTBEAT_TIMEOUT,      // Brak heartbeat
    ERR_SELF_TEST_FAILED        // Self-test nie przeszedł
};

// ============================================================================
// INFORMACJE O WZORCU MALOWANIA
// ============================================================================
struct PatternInfo {
    const char* name;           // Nazwa: P-1a, P-1b, etc.
    const char* desc;           // Opis po polsku
    float lineLength;           // Długość linii w metrach (0 = ciągła)
    float gapLength;            // Długość przerwy w metrach (0 = brak)
    int width;                  // Szerokość w cm (12 lub 24)
    bool guns[GUN_COUNT];       // Które pistolety [P1..P6] aktywne
};

// ============================================================================
// RAPORT PRACY
// ============================================================================
struct WorkReport {
    char startDate[11];                 // YYYY-MM-DD
    char startTime[9];                  // HH:MM:SS
    char endTime[9];                    // HH:MM:SS
    float areaPerPattern[PATTERN_COUNT]; // Powierzchnia per wzorzec [m²]
    float totalArea;                    // Suma powierzchni [m²]
    unsigned long workDuration;         // Czas pracy [sekundy]
};

// ============================================================================
// WPIS BŁĘDU SYSTEMOWEGO
// ============================================================================
struct SystemError {
    ErrorType type;         // Typ błędu
    char timestamp[20];     // YYYY-MM-DD HH:MM:SS
    char message[128];      // Opis błędu
};

#endif // TYPES_H
