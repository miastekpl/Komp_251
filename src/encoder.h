/**
 * @file encoder.h
 * @brief Enkoder pomiarowy - Trassar-Painter v7.0.0
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef ENCODER_H
#define ENCODER_H

void IRAM_ATTR encoderISR();
void IRAM_ATTR encoderButtonISR();

void updateDistanceAndSpeed();

void startCalibration();
void stopCalibration();

#endif // ENCODER_H
