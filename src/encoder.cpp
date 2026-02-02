/**
 * @file encoder.cpp
 * @brief Implementacja enkodera pomiarowego - Trassar-Painter v6.0.0
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

// Zewnętrzne obiekty
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

    // Start od przerwy - aktualizuj gapTraveled
    if (startFromGap && currentMode == MODE_WORKING) {
        float gapDistance = patterns[currentPattern].gapLength;
        if (gapDistance > 0 && gapTraveled < gapDistance) {
            gapTraveled = distanceTraveled;
            if (gapTraveled >= gapDistance) {
                Serial.println("[GAP] Przerwa przejechana - START malowania!");
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
    }
}

// ============================================================================
// KALIBRACJA AUTOMATYCZNA
// ============================================================================

void startCalibration() {
    currentMode = MODE_CALIBRATING;
    calibrationStartPulses = encoderPulses;
    distanceTraveled = 0.0f;
    updateGuns();  // Pistolety OFF

    Serial.println("[KALIBRACJA] START - jedz dokladnie 10 metrow");
    Serial.println("[KALIBRACJA] Wcisnij STOP gdy przejedziesz 10m");
    Serial.printf("[KALIBRACJA] Start impulsy: %ld\n", calibrationStartPulses);
}

void stopCalibration() {
    if (currentMode != MODE_CALIBRATING) return;

    long pulsesTotal = encoderPulses - calibrationStartPulses;

    Serial.printf("[KALIBRACJA] STOP - impulsy przejechane: %ld\n", pulsesTotal);

    if (pulsesTotal > 0) {
        encoderCalibration = pulsesTotal / CALIBRATION_DISTANCE_M;

        // Zapisz do NVS
        prefs.putFloat(NVS_KEY_CALIBRATION, encoderCalibration);

        Serial.printf("[KALIBRACJA] ZAPISANA: %.2f imp/m\n", encoderCalibration);
    } else {
        Serial.println("[KALIBRACJA] BLAD - brak impulsow! Sprawdz enkoder.");
    }

    currentMode = MODE_IDLE;
    updateGuns();
}
