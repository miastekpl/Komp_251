/**
 * @file sd_card.h
 * @brief Obsługa karty SD - Trassar-Painter v7.0.0
 *
 * Karta SD zintegrowana z wyświetlaczem TFT ILI9341.
 * Współdzieli magistralę SPI (MOSI=11, MISO=13, SCK=12).
 * Osobny pin CS: GPIO 2.
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef SD_CARD_H
#define SD_CARD_H

#include <Arduino.h>

// Inicjalizacja karty SD (wywołać po tftInit!)
bool initSDCard();

// Informacje o karcie
uint64_t getSDTotalMB();
uint64_t getSDUsedMB();

#endif // SD_CARD_H
