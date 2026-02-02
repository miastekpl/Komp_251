/**
 * @file buzzer_leds.cpp
 * @brief Implementacja buzzer i LED - Trassar-Painter v6.0.0
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#include <Arduino.h>
#include "buzzer_leds.h"
#include "pins.h"
#include "config.h"
#include "state.h"

// ============================================================================
// BUZZER
// ============================================================================

void buzzerBeep(int beeps) {
    buzzerBeepCount = beeps;
    buzzerActive = true;
    buzzerStartTime = millis();
}

void buzzerUpdate() {
    if (!buzzerActive) {
        digitalWrite(BUZZER_PIN, LOW);
        return;
    }

    unsigned long elapsed = millis() - buzzerStartTime;
    int beepCycle = (elapsed / BUZZER_CYCLE_MS) % 2;

    if (beepCycle == 0 && buzzerBeepCount > 0) {
        digitalWrite(BUZZER_PIN, HIGH);
    } else {
        digitalWrite(BUZZER_PIN, LOW);
        if (beepCycle == 1) {
            buzzerBeepCount--;
            if (buzzerBeepCount <= 0) {
                buzzerActive = false;
            }
        }
    }
}

// ============================================================================
// LEDy STATUSU
// ============================================================================

void updateStatusLeds() {
    digitalWrite(LED_STATUS_GREEN, statusLedGreen ? HIGH : LOW);
    digitalWrite(LED_STATUS_RED, statusLedRed ? HIGH : LOW);
    digitalWrite(LED_STATUS_YELLOW, statusLedYellow ? HIGH : LOW);
}

void setStatusLed(bool green, bool red, bool yellow) {
    statusLedGreen = green;
    statusLedRed = red;
    statusLedYellow = yellow;
    updateStatusLeds();
}
