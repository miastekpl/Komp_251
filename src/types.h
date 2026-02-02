/**
 * @file types.h
 * @brief Definicje typów, struktur i enumeracji - Trassar-Painter v7.0.0
 *
 * ZMIANY v7.0.0:
 * - Dodano ERR_DEADMAN_TIMEOUT, ERR_SPEED_TOO_LOW, ERR_SD_CARD_FAILED
 * - Dodano PatternCycleState (cyklowanie wzorców przerywanych)
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
    MODE_IDLE,
    MODE_WORKING,
    MODE_PAUSED,
    MODE_MEASURING,
    MODE_MENU,
    MODE_SERVICE,
    MODE_CALIBRATING,
    MODE_EMERGENCY
};

// ============================================================================
// TYPY BŁĘDÓW BEZPIECZEŃSTWA
// ============================================================================
enum ErrorType {
    ERR_NONE = 0,
    ERR_ESTOP_PRESSED,
    ERR_WATCHDOG_TIMEOUT,
    ERR_ENCODER_STALL,
    ERR_ENCODER_DISCONNECTED,
    ERR_SPEED_INVALID,
    ERR_SPEED_TOO_LOW,          // v7.0.0: prędkość < MIN_PAINTING_SPEED_KMH
    ERR_GUN_FEEDBACK,
    ERR_RTC_FAILED,
    ERR_FILESYSTEM_FULL,
    ERR_CALIBRATION_DRIFT,
    ERR_HEARTBEAT_TIMEOUT,
    ERR_DEADMAN_TIMEOUT,        // v7.0.0: dedykowany typ
    ERR_SELF_TEST_FAILED,
    ERR_SD_CARD_FAILED          // v7.0.0: błąd karty SD
};

// ============================================================================
// INFORMACJE O WZORCU MALOWANIA
// ============================================================================
struct PatternInfo {
    const char* name;
    const char* desc;
    float lineLength;           // 0 = ciągła
    float gapLength;            // 0 = brak przerwy
    int width;                  // cm (12 lub 24)
    bool guns[GUN_COUNT];
};

// ============================================================================
// STAN CYKLOWANIA WZORCA (NOWE v7.0.0)
// ============================================================================
struct PatternCycleState {
    float cycleDistance;         // Dystans w bieżącym cyklu linia+przerwa
    bool inLine;                // true = malujemy, false = przerwa
};

// ============================================================================
// RAPORT PRACY
// ============================================================================
struct WorkReport {
    char startDate[11];
    char startTime[9];
    char endTime[9];
    float areaPerPattern[PATTERN_COUNT];
    float totalArea;
    unsigned long workDuration;
};

// ============================================================================
// WPIS BŁĘDU SYSTEMOWEGO
// ============================================================================
struct SystemError {
    ErrorType type;
    char timestamp[20];
    char message[128];
};

#endif // TYPES_H
