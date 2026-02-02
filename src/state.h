/**
 * @file state.h
 * @brief Globalny stan systemu - Trassar-Painter v7.0.0
 *
 * ZMIANY v7.0.0:
 * - Usunięto flag_btnPause (jeden przycisk START/PAUZA)
 * - Dodano patternCycle (cyklowanie wzorców przerywanych)
 * - Dodano speedSufficient (prędkość > 3 km/h)
 * - Dodano sdCardAvailable
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef STATE_H
#define STATE_H

#include <Arduino.h>
#include "types.h"
#include "config.h"

// ============================================================================
// FLAGI ISR (volatile - modyfikowane w przerwaniach)
// ============================================================================
// v7.0.0: flag_btnStartPause zamiast osobnych flag_btnStart + flag_btnPause
extern volatile bool flag_btnStartPause;
extern volatile bool flag_btnStop;
extern volatile bool flag_encoderButton;
extern volatile bool flag_emergencyStop;

extern portMUX_TYPE isr_mux;

// ============================================================================
// TRYB PRACY
// ============================================================================
extern SystemMode currentMode;

// ============================================================================
// WZORZEC
// ============================================================================
extern int currentPattern;
extern bool selectorP3Physical;
extern bool selectorP3Virtual;

// ============================================================================
// CYKLOWANIE WZORCA (NOWE v7.0.0)
// ============================================================================
extern PatternCycleState patternCycle;

// ============================================================================
// PRĘDKOŚĆ WYSTARCZAJĄCA DO MALOWANIA (NOWE v7.0.0)
// ============================================================================
extern bool speedSufficient;   // true gdy speed >= MIN_PAINTING_SPEED_KMH

// ============================================================================
// PISTOLETY
// ============================================================================
extern bool gunsActive[GUN_COUNT];

// ============================================================================
// TRYB SERWISOWY
// ============================================================================
extern int serviceTestPattern;

// ============================================================================
// ENKODER POMIAROWY
// ============================================================================
extern volatile long encoderPulses;
extern float encoderCalibration;
extern float distanceTraveled;
extern float currentSpeed;
extern unsigned long lastSpeedCalc;
extern long lastPulseCount;
extern long calibrationStartPulses;

// ============================================================================
// JOYSTICK
// ============================================================================
extern int joyX;
extern int joyY;
extern bool joySW;

// ============================================================================
// START OD PRZERWY
// ============================================================================
extern bool startFromGap;
extern float gapTraveled;

// ============================================================================
// STATYSTYKI
// ============================================================================
extern unsigned long workStartTime;
extern unsigned long totalWorkTime;
extern int patternChangeCount;

// ============================================================================
// RAPORTY PRACY
// ============================================================================
extern WorkReport currentReport;
extern float distancePerPattern[PATTERN_COUNT];
extern bool reportActive;
extern int reportCount;

// ============================================================================
// MENU
// ============================================================================
extern int menuIndex;
extern const char* menuItems[];
extern const int menuItemsCount;

// ============================================================================
// SYSTEM BEZPIECZEŃSTWA
// ============================================================================
extern bool emergencyStopActive;
extern bool emergencyStopReleased;

extern bool watchdogEnabled;
extern unsigned long lastWatchdogReset;

extern unsigned long lastHeartbeat;

extern unsigned long lastDeadmanConfirm;
extern bool deadmanActive;

extern long lastEncoderPulses;
extern unsigned long lastEncoderChange;

extern float initialCalibration;

extern SystemError errorLog[ERROR_LOG_SIZE];
extern int errorLogIndex;
extern int errorCount;

extern bool statusLedGreen;
extern bool statusLedRed;
extern bool statusLedYellow;

extern unsigned long buzzerStartTime;
extern int buzzerBeepCount;
extern bool buzzerActive;

extern bool selfTestPassed;
extern char selfTestMessage[256];

// ============================================================================
// KARTA SD (NOWE v7.0.0)
// ============================================================================
extern bool sdCardAvailable;

// ============================================================================
// TFT
// ============================================================================
extern unsigned long lastTFTUpdate;

#endif // STATE_H
