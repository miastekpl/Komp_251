/**
 * @file display.cpp
 * @brief Implementacja wyświetlacza TFT ILI9341 - Trassar-Painter v7.0.0
 *
 * v7.0.0: Dodano informację o prędkości minimalnej i stanie cyklu wzorca
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "display.h"
#include "config.h"
#include "state.h"
#include "patterns.h"

extern TFT_eSPI tft;

void tftInit() {
    // Odkomentuj gdy podłączysz wyświetlacz TFT:
    /*
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(30, 100);
    tft.println("TRASSAR PAINTER");
    tft.setTextSize(1);
    tft.setCursor(80, 130);
    tft.printf("v%s", FIRMWARE_VERSION);
    delay(2000);
    tft.fillScreen(TFT_BLACK);
    */
    Serial.println("[TFT] TYMCZASOWO WYLACZONY");
}

void tftDrawStatus() {
    tft.fillRect(0, 0, TFT_SCREEN_WIDTH, 30, TFT_NAVY);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.setTextSize(2);
    tft.setCursor(5, 8);
    tft.print(getModeName(currentMode));
    tft.setCursor(150, 8);
    tft.print(patterns[currentPattern].name);

    tft.drawFastHLine(0, 30, TFT_SCREEN_WIDTH, TFT_WHITE);

    // Pomiary
    tft.setTextSize(3);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 40);
    tft.printf("%.2fm   ", distanceTraveled);

    tft.setCursor(10, 75);
    // v7.0.0: Kolor prędkości zależy od minimalnej
    if (currentSpeed >= MIN_PAINTING_SPEED_KMH) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
    }
    tft.printf("%.1fkm/h  ", currentSpeed);

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 110);
    tft.printf("Impulsy: %ld    ", encoderPulses);

    // v7.0.0: Stan cyklu wzorca
    if (currentMode == MODE_WORKING && isPatternDashed(currentPattern)) {
        tft.setCursor(10, 125);
        if (patternCycle.inLine) {
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.printf("LINIA: %.2f/%.1fm  ", patternCycle.cycleDistance, patterns[currentPattern].lineLength);
        } else {
            tft.setTextColor(TFT_ORANGE, TFT_BLACK);
            float gapStart = patterns[currentPattern].lineLength;
            tft.printf("PRZERWA: %.2f/%.1fm  ", patternCycle.cycleDistance - gapStart, patterns[currentPattern].gapLength);
        }
    }

    // Pistolety
    tft.drawFastHLine(0, 195, TFT_SCREEN_WIDTH, TFT_DARKGREY);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 200);
    tft.print("Pistolety:");

    for (int i = 0; i < GUN_COUNT; i++) {
        int x = 10 + i * 50;
        int y = 210;
        if (gunsActive[i]) {
            tft.fillRect(x, y, 45, 25, TFT_GREEN);
            tft.setTextColor(TFT_BLACK, TFT_GREEN);
        } else {
            tft.fillRect(x, y, 45, 25, TFT_DARKGREY);
            tft.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
        }
        tft.setTextSize(2);
        tft.setCursor(x + 12, y + 5);
        tft.printf("P%d", i + 1);
    }
}

void tftDrawMenu() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(80, 10);
    tft.println("MENU");

    for (int i = 0; i < menuItemsCount; i++) {
        int y = 40 + i * 30;
        if (i == menuIndex) {
            tft.fillRect(0, y, TFT_SCREEN_WIDTH, 25, TFT_NAVY);
            tft.setTextColor(TFT_YELLOW, TFT_NAVY);
        } else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        tft.setCursor(10, y + 5);
        tft.setTextSize(1);
        tft.println(menuItems[i]);
    }
}
