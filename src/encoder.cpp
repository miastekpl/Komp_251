/**
 * @file encoder.cpp
 * @brief Implementacja enkodera pomiarowego - Trassar-Painter v7.0.0
 *
 * ZMIANY v7.0.0:
 * - Aktualizacja patternCycle (cyklowanie wzorców przerywanych)
 * - Aktualizacja speedSufficient (kontrola prędkości malowania)
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#include <Arduino.h>
#include <Preferences.h>
#include "encoder.h"
#include "pins.h"
#include "config.h"
#include "state.h"
#include "patterns.h"

extern Preferences prefs;
extern void updateGuns();
extern void updatePatternDistance(float deltaDistance);

// ============================================================================
// ISR - PRZERWANIA ENKODERA
// ============================================================================

void IRAM_ATTR encoderISR() {
    portENTER_CRITICAL_ISR(&isr_mux);
    if (currentMode == MODE_WORKING || currentMode == MODE_MEASURING ||
        currentMode == MODE_CALIBRATING) {
        encoderPulses++;
    }
    portEXIT_CRITICAL_ISR(&isr_mux);
}

void IRAM_ATTR encoderButtonISR() {
    static unsigned long lastPress = 0;
    unsigned long now = millis();
    if (now - lastPress > BUTTON_DEBOUNCE_MS) {
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_encoderButton = true;
        portEXIT_CRITICAL_ISR(&isr_mux);
        lastPress = now;
    }
}

// ============================================================================
// DYSTANS I PRĘDKOŚĆ
// ============================================================================

void updateDistanceAndSpeed() {
    // Dystans
    float prevDistance = distanceTraveled;
    distanceTraveled = encoderPulses / encoderCalibration;
    float deltaDistance = distanceTraveled - prevDistance;

    // Aktualizuj dystans per wzorzec (raporty)
    if (deltaDistance > 0) {
        updatePatternDistance(deltaDistance);
    }

    // v7.0.0: CYKLOWANIE WZORCÓW PRZERYWANYCH
    if (currentMode == MODE_WORKING && deltaDistance > 0) {
        if (isPatternDashed(currentPattern)) {
            float lineLen = patterns[currentPattern].lineLength;
            float gapLen = patterns[currentPattern].gapLength;
            float cycleLen = lineLen + gapLen;

            patternCycle.cycleDistance += deltaDistance;

            // Sprawdź przejście między fazami
            bool wasInLine = patternCycle.inLine;

            if (patternCycle.cycleDistance >= cycleLen) {
                patternCycle.cycleDistance = fmod(patternCycle.cycleDistance, cycleLen);
            }

            patternCycle.inLine = (patternCycle.cycleDistance < lineLen);

            // Jeśli zmiana fazy - zaktualizuj pistolety
            if (wasInLine != patternCycle.inLine) {
                updateGuns();
                if (patternCycle.inLine) {
                    Serial.printf("[CYCLE] LINIA (%.1fm)\n", lineLen);
                } else {
                    Serial.printf("[CYCLE] PRZERWA (%.1fm)\n", gapLen);
                }
            }
        }
    }

    // Start od przerwy - aktualizuj gapTraveled
    if (startFromGap && currentMode == MODE_WORKING) {
        float gapDistance = patterns[currentPattern].gapLength;
        if (gapDistance > 0 && gapTraveled < gapDistance) {
            gapTraveled = distanceTraveled;
            if (gapTraveled >= gapDistance) {
                Serial.println("[GAP] Przerwa przejechana - START malowania!");
                startFromGap = false;
                patternCycle.cycleDistance = 0.0f;
                patternCycle.inLine = true;
                updateGuns();
            }
        }
    }

    // Prędkość (co 1s)
    if (millis() - lastSpeedCalc >= SPEED_CALC_INTERVAL_MS) {
        long pulseDiff = encoderPulses - lastPulseCount;
        float distance = pulseDiff / encoderCalibration;
        currentSpeed = distance * 3.6f;  // m/s -> km/h

        lastPulseCount = encoderPulses;
        lastSpeedCalc = millis();

        // v7.0.0: Aktualizuj flagę prędkości
        bool wasSufficient = speedSufficient;
        speedSufficient = (currentSpeed >= MIN_PAINTING_SPEED_KMH);

        // Jeśli zmiana stanu prędkości podczas malowania - aktualizuj pistolety
        if (currentMode == MODE_WORKING && wasSufficient != speedSufficient) {
            updateGuns();
            if (!speedSufficient) {
                Serial.printf("[SPEED] Za wolno! %.1f km/h < %.1f km/h - pistolety OFF\n",
                              currentSpeed, MIN_PAINTING_SPEED_KMH);
            } else {
                Serial.printf("[SPEED] Predkosc OK: %.1f km/h - pistolety ON\n", currentSpeed);
            }
        }
    }
}

// ============================================================================
// KALIBRACJA AUTOMATYCZNA
// ============================================================================

void startCalibration() {
    currentMode = MODE_CALIBRATING;
    calibrationStartPulses = encoderPulses;
    distanceTraveled = 0.0f;
    updateGuns();

    Serial.println("[KALIBRACJA] START - jedz dokladnie 10 metrow");
    Serial.println("[KALIBRACJA] Wcisnij STOP gdy przejedziesz 10m");
}

void stopCalibration() {
    if (currentMode != MODE_CALIBRATING) return;

    long pulsesTotal = encoderPulses - calibrationStartPulses;

    if (pulsesTotal > 0) {
        encoderCalibration = pulsesTotal / CALIBRATION_DISTANCE_M;
        prefs.putFloat(NVS_KEY_CALIBRATION, encoderCalibration);
        Serial.printf("[KALIBRACJA] ZAPISANA: %.2f imp/m\n", encoderCalibration);
    } else {
        Serial.println("[KALIBRACJA] BLAD - brak impulsow!");
    }

    currentMode = MODE_IDLE;
    updateGuns();
}
