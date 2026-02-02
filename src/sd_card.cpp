/**
 * @file sd_card.cpp
 * @brief Implementacja obsługi karty SD - Trassar-Painter v7.0.0
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "sd_card.h"
#include "pins.h"
#include "config.h"
#include "state.h"
#include "safety.h"

bool initSDCard() {
    Serial.println("[SD] Inicjalizacja karty SD...");
    Serial.printf("[SD] CS pin: GPIO %d, SPI: MOSI=11, MISO=13, SCK=12\n", SD_CS_PIN);

    // SD card używa tego samego SPI co TFT
    // TFT_eSPI już zainicjalizował SPI, więc SD.begin użyje istniejącego
    if (!SD.begin(SD_CS_PIN)) {
        Serial.println("[SD] Karta SD nie wykryta!");
        Serial.println("[SD] Raporty beda zapisywane na LittleFS (flash)");
        sdCardAvailable = false;
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] Brak karty w czytniku!");
        sdCardAvailable = false;
        return false;
    }

    const char* typeStr = "UNKNOWN";
    if (cardType == CARD_MMC) typeStr = "MMC";
    else if (cardType == CARD_SD) typeStr = "SDSC";
    else if (cardType == CARD_SDHC) typeStr = "SDHC";

    uint64_t totalMB = SD.totalBytes() / (1024 * 1024);
    uint64_t usedMB = SD.usedBytes() / (1024 * 1024);

    Serial.printf("[SD] Karta %s: %llu MB total, %llu MB used\n", typeStr, totalMB, usedMB);

    // Utwórz katalogi jeśli nie istnieją
    if (!SD.exists(SD_REPORTS_DIR)) {
        SD.mkdir(SD_REPORTS_DIR);
        Serial.printf("[SD] Utworzono katalog: %s\n", SD_REPORTS_DIR);
    }
    if (!SD.exists(SD_LOGS_DIR)) {
        SD.mkdir(SD_LOGS_DIR);
        Serial.printf("[SD] Utworzono katalog: %s\n", SD_LOGS_DIR);
    }

    sdCardAvailable = true;
    Serial.println("[SD] Karta SD gotowa - raporty beda zapisywane na SD");

    return true;
}

uint64_t getSDTotalMB() {
    if (!sdCardAvailable) return 0;
    return SD.totalBytes() / (1024 * 1024);
}

uint64_t getSDUsedMB() {
    if (!sdCardAvailable) return 0;
    return SD.usedBytes() / (1024 * 1024);
}
