/**
 * @file safety.cpp
 * @brief Implementacja systemu bezpieczeństwa - Trassar-Painter v6.0.0
 *
 * NAPRAWIONE w v6.0.0:
 * - activateEmergencyStop() używa RELAY_PINS[] zamiast RELAY_1+i
 * - checkHeartbeat() używa RELAY_PINS[] do wyłączenia pistoletów
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

// Zewnętrzne obiekty (z main.cpp)
extern RTC_DS1307 rtc;

// Forward declarations z innych modułów
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

    // Circular buffer
    errorLogIndex = (errorLogIndex + 1) % ERROR_LOG_SIZE;
    errorCount++;

    // Zapisz do pliku
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

    // NAPRAWIONE v6.0.0: Używamy tablicy RELAY_PINS[] zamiast RELAY_1+i
    // Bug w v5.3.0: setRelay(RELAY_1 + i) dawał piny 21,22,23,24,25,26
    // Prawdziwe piny to: 21,47,48,45,38,39
    for (int i = 0; i < RELAY_COUNT; i++) {
        gunsActive[i] = false;
        digitalWrite(RELAY_PINS[i], LOW);
    }

    // Zapisz raport jeśli aktywny
    if (reportActive) {
        stopReport();
    }

    // Alarm dźwiękowy - 5 krótkich sygnałów
    buzzerBeep(5);

    // LED - czerwony ciągły
    setStatusLed(false, true, false);

    logError(ERR_ESTOP_PRESSED, "EMERGENCY STOP PRESSED!");

    Serial.println("\n[E-STOP] EMERGENCY STOP AKTYWNY!");
    Serial.println("[E-STOP] Wszystkie pistolety WYLACZONE");
    Serial.println("[E-STOP] Zwolnij E-STOP i wcisnij START aby zresetowac");
}

void resetEmergencyStop() {
    if (!emergencyStopActive) return;

    // Sprawdź czy przycisk E-STOP jest zwolniony
    if (digitalRead(BTN_EMERGENCY_STOP) == LOW) {
        Serial.println("[E-STOP] Zwolnij przycisk E-STOP aby zresetowac!");
        buzzerBeep(2);
        return;
    }

    emergencyStopActive = false;
    emergencyStopReleased = true;
    currentMode = MODE_IDLE;

    setStatusLed(true, false, false);
    buzzerBeep(1);

    Serial.println("[E-STOP] ZRESETOWANY - system gotowy do pracy");
}

// ============================================================================
// WATCHDOG
// ============================================================================

void resetWatchdog() {
    if (watchdogEnabled) {
        lastWatchdogReset = millis();
    }
}

bool checkWatchdog() {
    if (!watchdogEnabled) return true;

    if (millis() - lastWatchdogReset > WATCHDOG_TIMEOUT_MS) {
        logError(ERR_WATCHDOG_TIMEOUT, "Watchdog timeout - system nie odpowiada");
        return false;
    }
    return true;
}

// ============================================================================
// HEARTBEAT (FAIL-SAFE)
// ============================================================================

void updateHeartbeat() {
    lastHeartbeat = millis();
}

bool checkHeartbeat() {
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL_MS * 2) {
        logError(ERR_HEARTBEAT_TIMEOUT, "Heartbeat timeout - fail-safe aktywny");

        // NAPRAWIONE v6.0.0: Używamy RELAY_PINS[]
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
        logError(ERR_HEARTBEAT_TIMEOUT, "Deadman switch timeout - operator nie potwierdzil");

        if (currentMode == MODE_WORKING) {
            pauseSystem();
        }

        buzzerBeep(3);
        setStatusLed(false, false, true);

        Serial.println("[DEADMAN] Timeout - potwierdzenie wymagane!");
        return false;
    }
    return true;
}

// ============================================================================
// ENCODER HEALTH
// ============================================================================

bool checkEncoderHealth() {
    if (currentMode != MODE_WORKING && currentMode != MODE_MEASURING &&
        currentMode != MODE_CALIBRATING) {
        return true;
    }

    bool healthy = true;

    // Sprawdź czy są impulsy
    if (encoderPulses == lastEncoderPulses) {
        unsigned long elapsed = millis() - lastEncoderChange;
        if (elapsed > ENCODER_STALL_TIMEOUT_MS) {
            if (currentSpeed < 0.5f) {
                logError(ERR_ENCODER_STALL, "Enkoder zatrzymany - maszyna stoi?");
                setStatusLed(false, false, true);
                healthy = false;
            }
        }
    } else {
        lastEncoderPulses = encoderPulses;
        lastEncoderChange = millis();
    }

    // Sprawdź odłączenie (0 impulsów po 10s pracy)
    if (currentMode == MODE_WORKING && encoderPulses == 0 &&
        millis() - workStartTime > ENCODER_DISCONNECT_MS) {
        logError(ERR_ENCODER_DISCONNECTED, "Enkoder odlaczony - brak impulsow!");
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
        snprintf(msg, sizeof(msg), "Predkosc nierealistyczna: %.1f km/h (limit: %.1f)",
                 currentSpeed, SPEED_MAX_KMH);
        logError(ERR_SPEED_INVALID, msg);

        if (currentMode == MODE_WORKING) {
            pauseSystem();
        }

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
        snprintf(msg, sizeof(msg), "Kalibracja dryftuje: %.1f%% (bylo: %.1f, jest: %.1f)",
                 driftPercent, initialCalibration, encoderCalibration);
        logError(ERR_CALIBRATION_DRIFT, msg);

        setStatusLed(false, false, true);
        buzzerBeep(2);

        Serial.printf("[CALIB] Drift %.1f%% - rozwaz rekalibracje\n", driftPercent);
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

    // Test 1: RTC
    Serial.print("[SELF-TEST] RTC DS1307... ");
    if (rtc.begin() && rtc.isrunning()) {
        Serial.println("OK");
        strcat(msg, "RTC OK\n");
    } else {
        Serial.println("FAILED");
        strcat(msg, "RTC FAILED\n");
        allPassed = false;
        logError(ERR_RTC_FAILED, "RTC nie dziala");
    }

    // Test 2: LittleFS
    Serial.print("[SELF-TEST] LittleFS... ");
    size_t total = LittleFS.totalBytes();
    size_t used = LittleFS.usedBytes();
    size_t freeSpace = total - used;
    if (freeSpace > SELFTEST_MIN_FREE_KB * 1024) {
        Serial.printf("OK (wolne: %u KB)\n", (unsigned)(freeSpace / 1024));
        char buf[64];
        snprintf(buf, sizeof(buf), "LittleFS OK (%u KB free)\n", (unsigned)(freeSpace / 1024));
        strcat(msg, buf);
    } else {
        Serial.printf("WARNING (wolne: %u KB)\n", (unsigned)(freeSpace / 1024));
        char buf[64];
        snprintf(buf, sizeof(buf), "LittleFS LOW (%u KB)\n", (unsigned)(freeSpace / 1024));
        strcat(msg, buf);
        logError(ERR_FILESYSTEM_FULL, "LittleFS malo miejsca");
    }

    // Test 3: Enkoder
    Serial.print("[SELF-TEST] Enkoder... ");
    int enc_clk = digitalRead(ENC_CLK);
    int enc_dt = digitalRead(ENC_DT);
    if (enc_clk == HIGH && enc_dt == HIGH) {
        Serial.println("OK (pullup aktywny)");
        strcat(msg, "Enkoder OK\n");
    } else {
        Serial.println("CHECK (sprawdz polaczenia)");
        strcat(msg, "Enkoder CHECK\n");
    }

    // Test 4: Przekaźniki (krótki puls testowy)
    Serial.print("[SELF-TEST] Przekazniki... ");
    for (int i = 0; i < RELAY_COUNT; i++) {
        digitalWrite(RELAY_PINS[i], HIGH);
        delay(50);
        digitalWrite(RELAY_PINS[i], LOW);
    }
    Serial.println("OK (6 przekaznikow)");
    strcat(msg, "Przekazniki OK\n");

    // Test 5: E-STOP
    Serial.print("[SELF-TEST] E-STOP... ");
    if (digitalRead(BTN_EMERGENCY_STOP) == HIGH) {
        Serial.println("OK (zwolniony)");
        strcat(msg, "E-STOP OK\n");
    } else {
        Serial.println("WCISNIETY!");
        strcat(msg, "E-STOP PRESSED!\n");
        allPassed = false;
    }

    // Test 6: Safety GPIO (LEDs + Buzzer)
    Serial.print("[SELF-TEST] Safety GPIO... ");
    digitalWrite(LED_STATUS_GREEN, HIGH);
    delay(100);
    digitalWrite(LED_STATUS_GREEN, LOW);
    digitalWrite(LED_STATUS_YELLOW, HIGH);
    delay(100);
    digitalWrite(LED_STATUS_YELLOW, LOW);
    digitalWrite(LED_STATUS_RED, HIGH);
    delay(100);
    digitalWrite(LED_STATUS_RED, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("OK (buzzer + LEDs)");
    strcat(msg, "Safety GPIO OK\n");

    // Podsumowanie
    if (allPassed) {
        Serial.println("[SELF-TEST] PASSED - system gotowy");
        setStatusLed(true, false, false);
        strcat(msg, "\nALL TESTS PASSED");
    } else {
        Serial.println("[SELF-TEST] WARNINGS - sprawdz ostrzezenia");
        setStatusLed(false, false, true);
        strcat(msg, "\nSOME TESTS FAILED");
        logError(ERR_SELF_TEST_FAILED, "Self-test wykryl problemy");
    }

    selfTestPassed = allPassed;
    strncpy(selfTestMessage, msg, sizeof(selfTestMessage) - 1);

    return allPassed;
}
