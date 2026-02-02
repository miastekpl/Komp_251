/**
 * @file display.cpp
 * @brief Implementacja wyświetlacza TFT ILI9341 - Trassar-Painter v6.0.0
 *
 * UWAGA: Funkcje TFT są gotowe do użycia. Aby aktywować wyświetlacz:
 * 1. Podłącz TFT ILI9341 zgodnie ze schematem w docs/SCHEMAT_POLACZEN.md
 * 2. Odkomentuj kod w tftInit() (sekcja inicjalizacji)
 * 3. Odkomentuj wywołanie tftDrawStatus()/tftDrawMenu() w loop() (main.cpp)
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

// Zewnętrzny obiekt TFT (z main.cpp)
extern TFT_eSPI tft;

// ============================================================================
// INICJALIZACJA TFT
// ============================================================================

void tftInit() {
    // TYMCZASOWO WYŁĄCZONE - odkomentuj gdy podłączysz wyświetlacz TFT
    /*
    tft.init();
    tft.setRotation(1);  // Landscape 320x240
    tft.fillScreen(TFT_BLACK);

    // Logo startowe
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(30, 100);
    tft.println("TRASSAR PAINTER");
    tft.setTextSize(1);
    tft.setCursor(80, 130);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.printf("v%s", FIRMWARE_VERSION);
    delay(2000);
    tft.fillScreen(TFT_BLACK);
    */

    Serial.println("[TFT] TYMCZASOWO WYLACZONY - odkomentuj gdy podlaczysz");
}

// ============================================================================
// EKRAN STATUSU
// ============================================================================

void tftDrawStatus() {
    // Nagłówek
    tft.fillRect(0, 0, TFT_SCREEN_WIDTH, 30, TFT_NAVY);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.setTextSize(2);
    tft.setCursor(5, 8);
    tft.print(getModeName(currentMode));

    // Wzorzec
    tft.setCursor(150, 8);
    tft.print(patterns[currentPattern].name);

    tft.drawFastHLine(0, 30, TFT_SCREEN_WIDTH, TFT_WHITE);

    // Pomiary
    tft.setTextSize(3);

    // Dystans
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(10, 40);
    tft.printf("%.2fm   ", distanceTraveled);

    // Prędkość
    tft.setCursor(10, 75);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.printf("%.1fkm/h  ", currentSpeed);

    // Impulsy
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(10, 110);
    tft.printf("Impulsy: %ld    ", encoderPulses);

    // Selektor P3
    tft.setCursor(10, 125);
    bool p3State = selectorP3Physical || selectorP3Virtual;
    tft.setTextColor(p3State ? TFT_ORANGE : TFT_CYAN, TFT_BLACK);
    tft.printf("P3: %s      ", p3State ? "ODWROCONE" : "NORMALNE");

    // Pasek postępu przerwy
    if (startFromGap && currentMode == MODE_WORKING) {
        float gapDistance = patterns[currentPattern].gapLength;
        if (gapDistance > 0) {
            int barWidth = (int)((gapTraveled / gapDistance) * 300);
            if (barWidth > 300) barWidth = 300;

            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setCursor(10, 145);
            tft.printf("Przerwa: %.1f/%.1fm  ", gapTraveled, gapDistance);

            tft.drawRect(10, 160, 300, 15, TFT_WHITE);
            tft.fillRect(11, 161, barWidth, 13, TFT_YELLOW);
        }
    }

    // Pistolety - wizualizacja na dole ekranu
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

// ============================================================================
// EKRAN MENU
// ============================================================================

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

    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(10, 220);
    tft.setTextSize(1);
    tft.println("Joystick: gora/dol, przycisk=OK");
}
