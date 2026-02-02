/**
 * @file buzzer_leds.h
 * @brief Buzzer i LEDy statusu - Trassar-Painter v6.0.0
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef BUZZER_LEDS_H
#define BUZZER_LEDS_H

void buzzerBeep(int beeps);
void buzzerUpdate();
void updateStatusLeds();
void setStatusLed(bool green, bool red, bool yellow);

#endif // BUZZER_LEDS_H
