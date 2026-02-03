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
    sdCardAvailable = false;  // Domyślnie wyłączona

    Serial.println("[SD] Inicjalizacja karty SD...");
    Serial.printf("[SD] CS pin: GPIO %d\n", SD_CS_PIN);
    Serial.flush();

    // Ustaw CS pin jako OUTPUT HIGH (deselect) przed inicjalizacją
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(100);

    // Inicjalizuj SPI explicytnie (HSPI na ESP32-S3)
    // Piny: MOSI=11, MISO=13, SCK=12
    SPI.begin(12, 13, 11, SD_CS_PIN);
    delay(100);

    // Próba inicjalizacji SD z timeout
    Serial.println("[SD] Proba SD.begin()...");
    Serial.flush();

    // SD.begin może zająć chwilę lub zawiesić się - dajemy timeout przez sprawdzenie
    if (!SD.begin(SD_CS_PIN, SPI, 4000000)) {  // 4MHz SPI speed - bezpieczna wartość
        Serial.println("[SD] Karta SD nie wykryta lub blad SPI");
        Serial.println("[SD] Raporty beda zapisywane na LittleFS (flash)");
        Serial.flush();
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] Brak karty w czytniku!");
        Serial.flush();
        return false;
    }

    const char* typeStr = "UNKNOWN";
    if (cardType == CARD_MMC) typeStr = "MMC";
    else if (cardType == CARD_SD) typeStr = "SDSC";
    else if (cardType == CARD_SDHC) typeStr = "SDHC";

    uint64_t totalMB = SD.totalBytes() / (1024 * 1024);
    uint64_t usedMB = SD.usedBytes() / (1024 * 1024);

    Serial.printf("[SD] Karta %s: %llu MB total, %llu MB used\n", typeStr, totalMB, usedMB);
    Serial.flush();

    // Utwórz katalogi jeśli nie istnieją
    if (!SD.exists(SD_REPORTS_DIR)) {
        if (SD.mkdir(SD_REPORTS_DIR)) {
            Serial.printf("[SD] Utworzono katalog: %s\n", SD_REPORTS_DIR);
        }
    }
    if (!SD.exists(SD_LOGS_DIR)) {
        if (SD.mkdir(SD_LOGS_DIR)) {
            Serial.printf("[SD] Utworzono katalog: %s\n", SD_LOGS_DIR);
        }
    }

    sdCardAvailable = true;
    Serial.println("[SD] Karta SD gotowa - raporty beda zapisywane na SD");
    Serial.flush();

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
