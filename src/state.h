/**
 * @file state.h
 * @brief Globalny stan systemu - Trassar-Painter v6.0.0
 *
 * Deklaracje extern dla wszystkich zmiennych stanu.
 * Definicje w state.cpp.
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
extern volatile bool flag_btnStart;
extern volatile bool flag_btnStop;
extern volatile bool flag_btnPause;
extern volatile bool flag_encoderButton;
extern volatile bool flag_emergencyStop;

// Mutex dla critical sections (ISR)
extern portMUX_TYPE isr_mux;

// ============================================================================
// TRYB PRACY
// ============================================================================
extern SystemMode currentMode;

// ============================================================================
// WZORZEC
// ============================================================================
extern int currentPattern;              // Indeks aktualnego wzorca (0-14)
extern bool selectorP3Physical;         // Stan fizycznego selektora P3
extern bool selectorP3Virtual;          // Stan wirtualnego selektora P3 (z panelu WWW)

// ============================================================================
// PISTOLETY
// ============================================================================
extern bool gunsActive[GUN_COUNT];      // Stan przekaźników pistoletów

// ============================================================================
// TRYB SERWISOWY
// ============================================================================
extern int serviceTestPattern;          // -1 = żaden, 0-14 = testowany wzorzec

// ============================================================================
// ENKODER POMIAROWY
// ============================================================================
extern volatile long encoderPulses;     // Zliczone impulsy (volatile - ISR)
extern float encoderCalibration;        // Impulsy/metr (kalibracja)
extern float distanceTraveled;          // Dystans w metrach
extern float currentSpeed;              // Prędkość km/h
extern unsigned long lastSpeedCalc;     // Timestamp ostatniej kalkulacji
extern long lastPulseCount;             // Impulsy przy ostatniej kalkulacji
extern long calibrationStartPulses;     // Impulsy na początku kalibracji

// ============================================================================
// JOYSTICK
// ============================================================================
extern int joyX;                        // 0-4095
extern int joyY;                        // 0-4095
extern bool joySW;                      // Przycisk

// ============================================================================
// START OD PRZERWY
// ============================================================================
extern bool startFromGap;               // Czy aktywny "start od przerwy"
extern float gapTraveled;               // Ile przejechano w przerwie

// ============================================================================
// STATYSTYKI
// ============================================================================
extern unsigned long workStartTime;     // Timestamp rozpoczęcia pracy
extern unsigned long totalWorkTime;     // Łączny czas pracy (ms)
extern int patternChangeCount;          // Liczba zmian wzorca

// ============================================================================
// RAPORTY PRACY
// ============================================================================
extern WorkReport currentReport;        // Bieżący raport
extern float distancePerPattern[PATTERN_COUNT]; // Dystans per wzorzec [m]
extern bool reportActive;               // Czy raport jest aktywny
extern int reportCount;                 // Ilość zapisanych raportów

// ============================================================================
// MENU
// ============================================================================
extern int menuIndex;                   // Aktualny indeks w menu
extern const char* menuItems[];         // Pozycje menu
extern const int menuItemsCount;

// ============================================================================
// SYSTEM BEZPIECZEŃSTWA
// ============================================================================
// Emergency Stop
extern bool emergencyStopActive;
extern bool emergencyStopReleased;

// Watchdog
extern bool watchdogEnabled;
extern unsigned long lastWatchdogReset;

// Heartbeat
extern unsigned long lastHeartbeat;

// Deadman switch
extern unsigned long lastDeadmanConfirm;
extern bool deadmanActive;

// Encoder health
extern long lastEncoderPulses;
extern unsigned long lastEncoderChange;

// Calibration drift
extern float initialCalibration;

// Error logging
extern SystemError errorLog[ERROR_LOG_SIZE];
extern int errorLogIndex;
extern int errorCount;

// Status LEDs
extern bool statusLedGreen;
extern bool statusLedRed;
extern bool statusLedYellow;

// Buzzer
extern unsigned long buzzerStartTime;
extern int buzzerBeepCount;
extern bool buzzerActive;

// Self-test
extern bool selfTestPassed;
extern char selfTestMessage[256];

// ============================================================================
// TFT
// ============================================================================
extern unsigned long lastTFTUpdate;

#endif // STATE_H
