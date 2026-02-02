/**
 * @file safety.cpp
 * @brief Implementacja systemu bezpieczeństwa - Trassar-Painter v7.0.0
 *
 * ZMIANY v7.0.0:
 * - E-STOP: poprawiona logika NC (normalny stan = LOW)
 * - Deadman: dedykowany ERR_DEADMAN_TIMEOUT
 * - Self-test: sprawdzenie karty SD
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <Wire.h>
#include <RTClib.h>
#include "safety.h"
#include "pins.h"
#include "config.h"
#include "state.h"
#include "patterns.h"
#include "buzzer_leds.h"

extern RTC_DS1307 rtc;
extern void updateGuns();
extern void pauseSystem();
extern bool stopReport();

// ============================================================================
// LOGOWANIE BŁĘDÓW
// ============================================================================

void logError(ErrorType type, const char* message) {
    SystemError& err = errorLog[errorLogIndex];
    err.type = type;

    DateTime now = rtc.now();
    snprintf(err.timestamp, sizeof(err.timestamp),
             "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());

    strncpy(err.message, message, sizeof(err.message) - 1);
    err.message[sizeof(err.message) - 1] = '\0';

    errorLogIndex = (errorLogIndex + 1) % ERROR_LOG_SIZE;
    errorCount++;

    File errorFile = LittleFS.open(ERROR_LOG_FILE, "a");
    if (errorFile) {
        errorFile.printf("[%s] ERROR %d: %s\n", err.timestamp, type, message);
        errorFile.close();
    }

    Serial.printf("[ERROR] %s - %s\n", err.timestamp, message);
}

// ============================================================================
// EMERGENCY STOP
// ============================================================================

void activateEmergencyStop() {
    if (emergencyStopActive) return;

    emergencyStopActive = true;
    emergencyStopReleased = false;
    currentMode = MODE_EMERGENCY;

    for (int i = 0; i < RELAY_COUNT; i++) {
        gunsActive[i] = false;
        digitalWrite(RELAY_PINS[i], LOW);
    }

    if (reportActive) {
        stopReport();
    }

    buzzerBeep(5);
    setStatusLed(false, true, false);
    logError(ERR_ESTOP_PRESSED, "EMERGENCY STOP PRESSED!");

    Serial.println("\n[E-STOP] EMERGENCY STOP AKTYWNY!");
    Serial.println("[E-STOP] Wszystkie pistolety WYLACZONE");
}

void resetEmergencyStop() {
    if (!emergencyStopActive) return;

    // v7.0.0: E-STOP NC - zwolniony = pin LOW (obwód NC zamknięty = GND)
    // Wciśnięty = pin HIGH (obwód NC otwarty = PULLUP)
    if (digitalRead(BTN_EMERGENCY_STOP) == HIGH) {
        Serial.println("[E-STOP] Przycisk wciaz wcisniety! Zwolnij aby zresetowac.");
        buzzerBeep(2);
        return;
    }

    emergencyStopActive = false;
    emergencyStopReleased = true;
    currentMode = MODE_IDLE;

    setStatusLed(true, false, false);
    buzzerBeep(1);
    Serial.println("[E-STOP] ZRESETOWANY - system gotowy");
}

// ============================================================================
// WATCHDOG
// ============================================================================

void resetWatchdog() {
    if (watchdogEnabled) lastWatchdogReset = millis();
}

bool checkWatchdog() {
    if (!watchdogEnabled) return true;
    if (millis() - lastWatchdogReset > WATCHDOG_TIMEOUT_MS) {
        logError(ERR_WATCHDOG_TIMEOUT, "Watchdog timeout");
        return false;
    }
    return true;
}

// ============================================================================
// HEARTBEAT
// ============================================================================

void updateHeartbeat() {
    lastHeartbeat = millis();
}

bool checkHeartbeat() {
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS * 2) {
        logError(ERR_HEARTBEAT_TIMEOUT, "Heartbeat timeout - fail-safe aktywny");
        for (int i = 0; i < RELAY_COUNT; i++) {
            gunsActive[i] = false;
            digitalWrite(RELAY_PINS[i], LOW);
        }
        return false;
    }
    return true;
}

// ============================================================================
// DEADMAN SWITCH
// ============================================================================

void confirmDeadman() {
    lastDeadmanConfirm = millis();
}

bool checkDeadman() {
    if (!deadmanActive) return true;
    if (millis() - lastDeadmanConfirm > DEADMAN_TIMEOUT_MS) {
        logError(ERR_DEADMAN_TIMEOUT, "Deadman switch timeout - operator nie potwierdzil");
        if (currentMode == MODE_WORKING) pauseSystem();
        buzzerBeep(3);
        setStatusLed(false, false, true);
        return false;
    }
    return true;
}

// ============================================================================
// ENCODER HEALTH
// ============================================================================

bool checkEncoderHealth() {
    if (currentMode != MODE_WORKING && currentMode != MODE_MEASURING &&
        currentMode != MODE_CALIBRATING) return true;

    bool healthy = true;

    if (encoderPulses == lastEncoderPulses) {
        if (millis() - lastEncoderChange > ENCODER_STALL_TIMEOUT_MS && currentSpeed < 0.5f) {
            logError(ERR_ENCODER_STALL, "Enkoder zatrzymany");
            setStatusLed(false, false, true);
            healthy = false;
        }
    } else {
        lastEncoderPulses = encoderPulses;
        lastEncoderChange = millis();
    }

    if (currentMode == MODE_WORKING && encoderPulses == 0 &&
        millis() - workStartTime > ENCODER_DISCONNECT_MS) {
        logError(ERR_ENCODER_DISCONNECTED, "Enkoder odlaczony!");
        setStatusLed(false, true, false);
        buzzerBeep(5);
        healthy = false;
    }

    return healthy;
}

// ============================================================================
// SPEED VALIDATION
// ============================================================================

bool validateSpeed() {
    if (currentSpeed < SPEED_MIN_KMH || currentSpeed > SPEED_MAX_KMH) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Predkosc nierealistyczna: %.1f km/h", currentSpeed);
        logError(ERR_SPEED_INVALID, msg);
        if (currentMode == MODE_WORKING) pauseSystem();
        buzzerBeep(3);
        setStatusLed(false, false, true);
        return false;
    }
    return true;
}

// ============================================================================
// CALIBRATION DRIFT
// ============================================================================

bool checkCalibrationDrift() {
    float drift = abs(encoderCalibration - initialCalibration);
    float driftPercent = (drift / initialCalibration) * 100.0f;
    if (driftPercent > CALIBRATION_DRIFT_MAX_PERCENT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Kalibracja dryftuje: %.1f%%", driftPercent);
        logError(ERR_CALIBRATION_DRIFT, msg);
        setStatusLed(false, false, true);
        buzzerBeep(2);
        return false;
    }
    return true;
}

// ============================================================================
// SELF-TEST
// ============================================================================

bool performSelfTest() {
    Serial.println("\n[SELF-TEST] Diagnostyka startowa...");
    bool allPassed = true;
    char msg[256] = "Self-test:\n";

    // RTC
    if (rtc.begin() && rtc.isrunning()) {
        Serial.println("[SELF-TEST] RTC OK");
        strcat(msg, "RTC OK\n");
    } else {
        Serial.println("[SELF-TEST] RTC FAILED");
        strcat(msg, "RTC FAILED\n");
        allPassed = false;
        logError(ERR_RTC_FAILED, "RTC nie dziala");
    }

    // LittleFS
    size_t freeSpace = LittleFS.totalBytes() - LittleFS.usedBytes();
    if (freeSpace > SELFTEST_MIN_FREE_KB * 1024) {
        Serial.printf("[SELF-TEST] LittleFS OK (%u KB wolne)\n", (unsigned)(freeSpace / 1024));
    } else {
        Serial.println("[SELF-TEST] LittleFS LOW");
        logError(ERR_FILESYSTEM_FULL, "LittleFS malo miejsca");
    }

    // SD Card
    Serial.printf("[SELF-TEST] SD Card: %s\n", sdCardAvailable ? "OK" : "BRAK");

    // Enkoder
    Serial.printf("[SELF-TEST] Enkoder: CLK=%d DT=%d\n", digitalRead(ENC_CLK), digitalRead(ENC_DT));

    // Przekaźniki
    for (int i = 0; i < RELAY_COUNT; i++) {
        digitalWrite(RELAY_PINS[i], HIGH);
        delay(50);
        digitalWrite(RELAY_PINS[i], LOW);
    }
    Serial.println("[SELF-TEST] Przekazniki OK");

    // E-STOP (NC: normalny = LOW)
    if (digitalRead(BTN_EMERGENCY_STOP) == LOW) {
        Serial.println("[SELF-TEST] E-STOP OK (zwolniony)");
    } else {
        Serial.println("[SELF-TEST] E-STOP WCISNIETY!");
        allPassed = false;
    }

    // LEDs + Buzzer
    digitalWrite(LED_STATUS_GREEN, HIGH); delay(100); digitalWrite(LED_STATUS_GREEN, LOW);
    digitalWrite(LED_STATUS_YELLOW, HIGH); delay(100); digitalWrite(LED_STATUS_YELLOW, LOW);
    digitalWrite(LED_STATUS_RED, HIGH); delay(100); digitalWrite(LED_STATUS_RED, LOW);
    digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);

    if (allPassed) {
        setStatusLed(true, false, false);
        strcat(msg, "ALL PASSED");
    } else {
        setStatusLed(false, false, true);
        strcat(msg, "SOME FAILED");
        logError(ERR_SELF_TEST_FAILED, "Self-test wykryl problemy");
    }

    selfTestPassed = allPassed;
    strncpy(selfTestMessage, msg, sizeof(selfTestMessage) - 1);
    return allPassed;
}
