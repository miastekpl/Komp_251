/**
 * @file encoder.h
 * @brief Enkoder pomiarowy - Trassar-Painter v6.0.0
 *
 * Obsługa enkodera kwadraturowego, kalibracja, dystans, prędkość.
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef ENCODER_H
#define ENCODER_H

// ISR enkodera (IRAM_ATTR - musi być w IRAM)
void IRAM_ATTR encoderISR();
void IRAM_ATTR encoderButtonISR();

// Aktualizacja dystansu i prędkości (wywoływać w loop)
void updateDistanceAndSpeed();

// Kalibracja automatyczna
void startCalibration();
void stopCalibration();

#endif // ENCODER_H
