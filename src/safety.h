/**
 * @file safety.h
 * @brief System bezpieczeństwa - Trassar-Painter v6.0.0
 *
 * E-STOP, Watchdog, Heartbeat, Deadman, Self-test, Error logging.
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef SAFETY_H
#define SAFETY_H

#include "types.h"

// Logowanie błędów (circular buffer + plik)
void logError(ErrorType type, const char* message);

// Emergency Stop
void activateEmergencyStop();
void resetEmergencyStop();

// Watchdog
void resetWatchdog();
bool checkWatchdog();

// Heartbeat (fail-safe)
void updateHeartbeat();
bool checkHeartbeat();

// Deadman switch
void confirmDeadman();
bool checkDeadman();

// Encoder health
bool checkEncoderHealth();

// Speed validation
bool validateSpeed();

// Calibration drift
bool checkCalibrationDrift();

// Self-test diagnostyczny
bool performSelfTest();

#endif // SAFETY_H
