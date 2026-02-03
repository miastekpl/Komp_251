// ABSOLUTNE MINIMUM - tylko delay
#include <Arduino.h>

void setup() {
    delay(1000);
    Serial.begin(115200);
    delay(1000);
    Serial.println("ESP32 DZIALA!");
}

void loop() {
    Serial.println("loop");
    delay(1000);
}
