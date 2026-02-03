/**
 * @file main.cpp
 * @brief Trassar-Painter v7.0.0 - DIAGNOSTIC BUILD
 *
 * WERSJA DIAGNOSTYCZNA - minimalna ilość kodu do znalezienia przyczyny crash
 * Po znalezieniu problemu - przywróć pełny kod
 */

#include <Arduino.h>

// ============================================================================
// ABSOLUTNE MINIMUM - BEZ ŻADNYCH BIBLIOTEK
// ============================================================================

void setup() {
    // Długi delay PRZED czymkolwiek
    delay(5000);

    Serial.begin(115200);

    // Czekaj na Serial
    unsigned long start = millis();
    while (!Serial && millis() - start < 10000) {
        delay(100);
    }

    delay(2000);

    Serial.println("\n\n========================================");
    Serial.println("  TRASSAR DIAGNOSTIC BUILD");
    Serial.println("  Jesli widzisz ten tekst - ESP DZIALA!");
    Serial.println("========================================\n");

    Serial.println("[OK] Serial dziala");
    Serial.println("[TEST] Sprawdzanie podstawowych funkcji...\n");

    // Test GPIO
    Serial.println("[GPIO] Test pinow wyjsciowych...");
    int testPins[] = {21, 47, 48, 19, 38, 39, 35, 36, 37, 46};
    for (int i = 0; i < 10; i++) {
        pinMode(testPins[i], OUTPUT);
        digitalWrite(testPins[i], LOW);
        Serial.printf("  GPIO %d: OK\n", testPins[i]);
    }
    Serial.println("[GPIO] Wszystkie piny OK\n");

    // Test pamięci
    Serial.printf("[MEM] Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("[MEM] Min free heap: %d bytes\n", ESP.getMinFreeHeap());
    Serial.printf("[MEM] Heap size: %d bytes\n", ESP.getHeapSize());

    // Chip info
    Serial.printf("\n[CHIP] Model: %s\n", ESP.getChipModel());
    Serial.printf("[CHIP] Revision: %d\n", ESP.getChipRevision());
    Serial.printf("[CHIP] Cores: %d\n", ESP.getChipCores());
    Serial.printf("[CHIP] CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("[CHIP] Flash: %d MB\n", ESP.getFlashChipSize() / 1024 / 1024);

    Serial.println("\n========================================");
    Serial.println("  DIAGNOSTIC BUILD GOTOWY");
    Serial.println("  Jesli to dziala - problem w bibliotekach");
    Serial.println("========================================\n");
}

void loop() {
    static unsigned long lastPrint = 0;
    static int counter = 0;

    if (millis() - lastPrint > 2000) {
        lastPrint = millis();
        counter++;
        Serial.printf("[LOOP %d] Uptime: %lu ms, Heap: %d\n",
                      counter, millis(), ESP.getFreeHeap());
    }

    delay(10);
}
