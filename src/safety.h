/**
 * @file safety.h
 * @brief System bezpieczeństwa - Trassar-Painter v7.0.0
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef SAFETY_H
#define SAFETY_H

#include "types.h"

void logError(ErrorType type, const char* message);
void activateEmergencyStop();
void resetEmergencyStop();
void resetWatchdog();
bool checkWatchdog();
void updateHeartbeat();
bool checkHeartbeat();
void confirmDeadman();
bool checkDeadman();
bool checkEncoderHealth();
bool validateSpeed();
bool checkCalibrationDrift();
bool performSelfTest();

#endif // SAFETY_H
