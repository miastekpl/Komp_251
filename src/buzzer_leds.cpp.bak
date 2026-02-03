/**
 * @file buzzer_leds.cpp
 * @brief Implementacja buzzer i LED - Trassar-Painter v7.0.0
 *
 * v7.0.0: Poprawiona logika buzzerUpdate() - czysty licznik cykli
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
// BUZZER (v7.0.0: poprawiona logika)
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
    unsigned long fullCycleMs = BUZZER_CYCLE_MS * 2;  // on + off

    // Numer bieżącego beep-a (0-indexed)
    int currentBeep = elapsed / fullCycleMs;

    // Czy wszystkie beepy wykonane?
    if (currentBeep >= buzzerBeepCount) {
        buzzerActive = false;
        digitalWrite(BUZZER_PIN, LOW);
        return;
    }

    // Pozycja w bieżącym cyklu
    unsigned long posInCycle = elapsed % fullCycleMs;
    bool on = (posInCycle < (unsigned long)BUZZER_CYCLE_MS);

    digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
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
