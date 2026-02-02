/**
 * @file main.cpp
 * @brief Trassar-Painter v7.0.0 - SD-SPEED EDITION
 *
 * ═══════════════════════════════════════════════════════════════════
 * KOMPUTER MALOWARKI DROGOWEJ - WERSJA 7.0.0
 * ═══════════════════════════════════════════════════════════════════
 *
 * ZMIANY v7.0.0:
 * - JEDEN przycisk START/PAUZA (GPIO 40) zamiast osobnych START + PAUSE
 * - Malowanie wymaga prędkości >= 3 km/h (bezpieczeństwo)
 * - Cyklowanie wzorców przerywanych (linia/przerwa)
 * - E-STOP: ISR na CHANGE + sprawdzanie stanu pinu (NC)
 * - Karta SD do raportów (fallback na LittleFS)
 * - RELAY_4 przeniesiony z GPIO 45 na GPIO 19
 * - NTPClient usunięty - tylko configTime()
 *
 * @author Trassar251
 * @date 2026-02-02
 * @version 7.0.0
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <RTClib.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <Wire.h>

// Moduły Trassar-Painter
#include "pins.h"
#include "config.h"
#include "types.h"
#include "state.h"
#include "patterns.h"
#include "safety.h"
#include "encoder.h"
#include "reports.h"
#include "rtc_ntp.h"
#include "buzzer_leds.h"
#include "display.h"
#include "web_panel.h"
#include "sd_card.h"

// ============================================================================
// OBIEKTY GLOBALNE
// ============================================================================

TFT_eSPI tft = TFT_eSPI();
WebServer server(80);
Preferences prefs;
RTC_DS1307 rtc;

// ============================================================================
// ISR PRZYCISKÓW
// ============================================================================

// v7.0.0: JEDEN przycisk START/PAUZA
void IRAM_ATTR btnStartPauseISR() {
    static unsigned long lastPress = 0;
    unsigned long now = millis();
    if (now - lastPress > BUTTON_DEBOUNCE_MS) {
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_btnStartPause = true;
        portEXIT_CRITICAL_ISR(&isr_mux);
        lastPress = now;
    }
}

void IRAM_ATTR btnStopISR() {
    static unsigned long lastPress = 0;
    unsigned long now = millis();
    if (now - lastPress > BUTTON_DEBOUNCE_MS) {
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_btnStop = true;
        portEXIT_CRITICAL_ISR(&isr_mux);
        lastPress = now;
    }
}

// v7.0.0: E-STOP na CHANGE - sprawdzamy stan pinu w ISR
// NC button: normalny = LOW (obwód zamknięty), wciśnięty = HIGH (obwód otwarty)
void IRAM_ATTR emergencyStopISR() {
    if (digitalRead(BTN_EMERGENCY_STOP) == HIGH) {
        // Pin HIGH = E-STOP wciśnięty (NC otworzony)
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_emergencyStop = true;
        portEXIT_CRITICAL_ISR(&isr_mux);
    }
}

// ============================================================================
// FUNKCJE STERUJĄCE
// ============================================================================

void setRelay(int relayPin, bool state) {
    digitalWrite(relayPin, state ? HIGH : LOW);
}

/**
 * Aktualizuj pistolety wg: trybu, wzorca, selektora P3,
 * cyklu wzorca (linia/przerwa), prędkości
 */
void updateGuns() {
    if (currentMode == MODE_WORKING) {
        bool reverseP3 = selectorP3Physical || selectorP3Virtual;

        for (int i = 0; i < GUN_COUNT; i++) {
            gunsActive[i] = patterns[currentPattern].guns[i];
        }

        // Selektor P3: odwraca P1 <-> P3 dla wzorców podwójnych (7,8,9)
        if ((currentPattern == 7 || currentPattern == 8 || currentPattern == 9) && reverseP3) {
            bool temp = gunsActive[0];
            gunsActive[0] = gunsActive[2];
            gunsActive[2] = temp;
        }

        // v7.0.0: CYKLOWANIE WZORCÓW - wyłącz pistolety w fazie przerwy
        if (isPatternDashed(currentPattern) && !patternCycle.inLine) {
            for (int i = 0; i < GUN_COUNT; i++) gunsActive[i] = false;
        }

        // Start od przerwy - pistolety OFF do przejechania gapLength
        float gapDistance = patterns[currentPattern].gapLength;
        if (startFromGap && gapDistance > 0 && gapTraveled < gapDistance) {
            for (int i = 0; i < GUN_COUNT; i++) gunsActive[i] = false;
        }

        // v7.0.0: KONTROLA PRĘDKOŚCI - pistolety OFF gdy < 3 km/h
        if (!speedSufficient) {
            for (int i = 0; i < GUN_COUNT; i++) gunsActive[i] = false;
        }

    } else if (currentMode == MODE_SERVICE) {
        if (serviceTestPattern >= 0 && serviceTestPattern < PATTERN_COUNT) {
            for (int i = 0; i < GUN_COUNT; i++) {
                gunsActive[i] = patterns[serviceTestPattern].guns[i];
            }
        } else {
            for (int i = 0; i < GUN_COUNT; i++) gunsActive[i] = false;
        }
    } else {
        for (int i = 0; i < GUN_COUNT; i++) gunsActive[i] = false;
    }

    // Fizycznie ustaw przekaźniki
    for (int i = 0; i < RELAY_COUNT; i++) {
        setRelay(RELAY_PINS[i], gunsActive[i]);
    }
}

/**
 * v7.0.0: START malowania - wymaga trybu IDLE lub PAUSED
 */
void startSystem() {
    if (currentMode == MODE_IDLE || currentMode == MODE_PAUSED) {
        bool wasIdle = (currentMode == MODE_IDLE);

        currentMode = MODE_WORKING;
        workStartTime = millis();

        if (wasIdle) {
            encoderPulses = 0;
            distanceTraveled = 0.0f;
            gapTraveled = 0.0f;
            // Reset cyklu wzorca
            patternCycle.cycleDistance = 0.0f;
            patternCycle.inLine = true;
            startReport();
        }

        updateGuns();

        Serial.printf("[START] Malowanie: %s (%s)\n",
                      patterns[currentPattern].name, patterns[currentPattern].desc);

        if (!speedSufficient) {
            Serial.printf("[START] UWAGA: Prędkość < %.1f km/h - pistolety będą OFF\n",
                          MIN_PAINTING_SPEED_KMH);
        }

        if (startFromGap) {
            Serial.printf("[START] Start od przerwy: %.1f m\n",
                          patterns[currentPattern].gapLength);
        }
    }
}

void startMeasuring() {
    currentMode = MODE_MEASURING;
    encoderPulses = 0;
    distanceTraveled = 0.0f;
    updateGuns();
    Serial.println("[POMIAR] Start pomiaru - pistolety OFF");
}

void startService() {
    currentMode = MODE_SERVICE;
    serviceTestPattern = -1;
    updateGuns();
    Serial.println("[SERVICE] Tryb serwisowy");
}

void enterMenu() {
    currentMode = MODE_MENU;
    menuIndex = 0;
    Serial.println("[MENU] Wejscie do menu");
}

void stopSystem() {
    if (currentMode == MODE_IDLE) return;

    if (currentMode == MODE_CALIBRATING) {
        stopCalibration();
        return;
    }

    if (currentMode == MODE_WORKING) {
        totalWorkTime += (millis() - workStartTime);
        stopReport();
    }

    currentMode = MODE_IDLE;
    serviceTestPattern = -1;
    patternCycle.cycleDistance = 0.0f;
    patternCycle.inLine = true;
    updateGuns();

    Serial.printf("[STOP] Zatrzymany (dystans: %.2f m)\n", distanceTraveled);
}

/**
 * v7.0.0: PAUZA - wywoływana przez ten sam przycisk co START
 */
void pauseSystem() {
    if (currentMode == MODE_WORKING) {
        totalWorkTime += (millis() - workStartTime);
        currentMode = MODE_PAUSED;
        updateGuns();
        Serial.println("[PAUZA]");
    } else if (currentMode == MODE_PAUSED) {
        currentMode = MODE_WORKING;
        workStartTime = millis();
        updateGuns();
        Serial.println("[WZNOWIENIE]");
    }
}

void changePattern(int newPattern) {
    if (newPattern >= 0 && newPattern < PATTERN_COUNT) {
        currentPattern = newPattern;
        patternChangeCount++;
        // Reset cyklu wzorca przy zmianie
        patternCycle.cycleDistance = 0.0f;
        patternCycle.inLine = true;
        Serial.printf("[PATTERN] %s (%s)\n",
                      patterns[currentPattern].name, patterns[currentPattern].desc);
        if (currentMode == MODE_WORKING) updateGuns();
    }
}

void updateJoystick() {
    joyX = analogRead(JOY_VRX);
    joyY = analogRead(JOY_VRY);
    joySW = digitalRead(JOY_SW) == LOW;

    if (currentMode == MODE_MENU) {
        static unsigned long lastJoyMove = 0;
        static bool lastJoySW = false;

        if (millis() - lastJoyMove > JOY_MENU_DELAY_MS) {
            if (joyY < (JOY_CENTER - JOY_DEADZONE)) {
                menuIndex--;
                if (menuIndex < 0) menuIndex = menuItemsCount - 1;
                lastJoyMove = millis();
            } else if (joyY > (JOY_CENTER + JOY_DEADZONE)) {
                menuIndex++;
                if (menuIndex >= menuItemsCount) menuIndex = 0;
                lastJoyMove = millis();
            }
        }

        if (joySW && !lastJoySW) {
            Serial.printf("[MENU] Wybrano: %s\n", menuItems[menuIndex]);
            if (menuIndex == 0) startCalibration();
            else if (menuIndex == 6) startService();
            else currentMode = MODE_IDLE;
        }
        lastJoySW = joySW;
    }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    Serial.println("\n================================================");
    Serial.printf("  TRASSAR PAINTER v%s\n", FIRMWARE_VERSION);
    Serial.printf("  %s\n", FIRMWARE_CODENAME);
    Serial.println("  Jeden przycisk START/PAUZA");
    Serial.printf("  Min. predkosc malowania: %.1f km/h\n", MIN_PAINTING_SPEED_KMH);
    Serial.println("  SD Card + E-STOP NC + Cyklowanie wzorc.");
    Serial.println("================================================\n");

    // LittleFS (zawsze - error log)
    if (!LittleFS.begin(true)) {
        Serial.println("[FS] Blad LittleFS");
    } else {
        Serial.println("[FS] LittleFS OK");
        reportCount = 0;
        while (LittleFS.exists("/report_" + String(reportCount) + ".txt")) {
            reportCount++;
        }
        Serial.printf("[FS] Znaleziono %d raportow (LittleFS)\n", reportCount);
    }

    // TFT
    tftInit();

    // SD Card (v7.0.0 - po TFT init bo współdzielą SPI)
    initSDCard();
    if (sdCardAvailable) {
        // Przelicz raporty na SD
        int sdReportCount = 0;
        while (SD.exists(String(SD_REPORTS_DIR) + "/report_" + String(sdReportCount) + ".txt")) {
            sdReportCount++;
        }
        if (sdReportCount > 0) {
            reportCount = sdReportCount;
            Serial.printf("[SD] Znaleziono %d raportow na SD\n", reportCount);
        }
    }

    // Preferences
    prefs.begin(NVS_NAMESPACE, false);
    encoderCalibration = prefs.getFloat(NVS_KEY_CALIBRATION, DEFAULT_CALIBRATION);
    Serial.printf("[CALIB] Enkoder: %.1f imp/m\n", encoderCalibration);

    // GPIO - Przekaźniki
    for (int i = 0; i < RELAY_COUNT; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], LOW);
    }

    // GPIO - Przyciski (v7.0.0: jeden START/PAUZA, brak osobnego PAUSE)
    pinMode(BTN_START_PAUSE, INPUT_PULLUP);
    pinMode(BTN_STOP, INPUT_PULLUP);
    Serial.println("[GPIO] Przyciski: START/PAUZA(40), STOP(41)");

    // GPIO - Selektor, Joystick, Enkoder
    pinMode(SEL_P3, INPUT_PULLUP);
    pinMode(JOY_SW, INPUT_PULLUP);
    pinMode(ENC_CLK, INPUT_PULLUP);
    pinMode(ENC_DT, INPUT_PULLUP);
    pinMode(ENC_SW, INPUT_PULLUP);

    // GPIO - Safety
    pinMode(BTN_EMERGENCY_STOP, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_STATUS_GREEN, OUTPUT);
    pinMode(LED_STATUS_RED, OUTPUT);
    pinMode(LED_STATUS_YELLOW, OUTPUT);

    // RTC
    initRTC();

    // Przerwania - Enkoder
    attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR, RISING);
    attachInterrupt(digitalPinToInterrupt(ENC_SW), encoderButtonISR, FALLING);

    // v7.0.0: JEDEN przycisk START/PAUZA
    attachInterrupt(digitalPinToInterrupt(BTN_START_PAUSE), btnStartPauseISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_STOP), btnStopISR, FALLING);

    // v7.0.0: E-STOP na CHANGE (NC: LOW=normal, HIGH=pressed)
    attachInterrupt(digitalPinToInterrupt(BTN_EMERGENCY_STOP), emergencyStopISR, CHANGE);
    Serial.println("[INT] Przerwania OK");

    // Kalibracja drift
    initialCalibration = encoderCalibration;

    // Self-test
    performSelfTest();

    // Timery safety
    lastWatchdogReset = millis();
    lastHeartbeat = millis();
    lastDeadmanConfirm = millis();
    lastEncoderChange = millis();

    // WiFi AP+STA
    WiFi.mode(WIFI_AP_STA);

    Serial.printf("[WiFi STA] Laczenie z %s...\n", WIFI_STA_SSID);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi STA] IP: %s\n", WiFi.localIP().toString().c_str());
        syncNTP();
    } else {
        Serial.println("\n[WiFi STA] Brak polaczenia");
    }

    // AP
    if (WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS)) {
        Serial.printf("[WiFi AP] SSID: %s, IP: %s\n",
                      WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());

        // OTA
        ArduinoOTA.setHostname(OTA_HOSTNAME);
        ArduinoOTA.setPassword(OTA_PASSWORD);
        ArduinoOTA.onStart([]() {
            LittleFS.end();
            Serial.println("[OTA] Start");
        });
        ArduinoOTA.onEnd([]() { Serial.println("[OTA] Zakonczone!"); });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[OTA] %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("[OTA] Blad[%u]\n", error);
        });
        ArduinoOTA.begin();

        // Web Server
        setupWebServer();

        Serial.println("\n================================================");
        Serial.println("  PANEL: http://192.168.4.1");
        Serial.printf("  OTA: %s / %s\n", OTA_HOSTNAME, OTA_PASSWORD);
        Serial.println("================================================\n");
    }

    Serial.println("[SYSTEM] GOTOWY DO PRACY\n");
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
    // ═══════════════════════════════════════════════════════════════
    // 1. SAFETY - NAJWYŻSZY PRIORYTET
    // ═══════════════════════════════════════════════════════════════

    if (flag_emergencyStop) {
        portENTER_CRITICAL(&isr_mux);
        flag_emergencyStop = false;
        portEXIT_CRITICAL(&isr_mux);
        activateEmergencyStop();
    }

    if (emergencyStopActive) {
        buzzerUpdate();
        updateStatusLeds();
        server.handleClient();
        delay(50);
        return;
    }

    resetWatchdog();
    updateHeartbeat();
    buzzerUpdate();

    // ═══════════════════════════════════════════════════════════════
    // 2. WEB & OTA
    // ═══════════════════════════════════════════════════════════════

    ArduinoOTA.handle();
    server.handleClient();

    // ═══════════════════════════════════════════════════════════════
    // 3. OBSŁUGA FLAG ISR
    // ═══════════════════════════════════════════════════════════════

    // v7.0.0: JEDEN przycisk START/PAUZA
    if (flag_btnStartPause) {
        portENTER_CRITICAL(&isr_mux);
        flag_btnStartPause = false;
        portEXIT_CRITICAL(&isr_mux);

        if (currentMode == MODE_WORKING) {
            pauseSystem();      // Jeśli maluje -> pauza
        } else if (currentMode == MODE_IDLE || currentMode == MODE_PAUSED) {
            startSystem();      // Jeśli idle/pauza -> start
        }
        confirmDeadman();
    }

    if (flag_btnStop) {
        portENTER_CRITICAL(&isr_mux);
        flag_btnStop = false;
        portEXIT_CRITICAL(&isr_mux);
        stopSystem();
    }

    if (flag_encoderButton) {
        portENTER_CRITICAL(&isr_mux);
        flag_encoderButton = false;
        portEXIT_CRITICAL(&isr_mux);

        if (currentMode == MODE_MENU) {
            Serial.printf("[MENU] Wybrano: %s\n", menuItems[menuIndex]);
        } else {
            pauseSystem();
        }
        confirmDeadman();
    }

    // ═══════════════════════════════════════════════════════════════
    // 4. POMIARY I WEJŚCIA
    // ═══════════════════════════════════════════════════════════════

    updateDistanceAndSpeed();
    updateJoystick();

    // Selektor P3 fizyczny
    static bool lastP3State = false;
    bool currentP3State = digitalRead(SEL_P3) == HIGH;
    if (currentP3State != lastP3State) {
        lastP3State = currentP3State;
        selectorP3Physical = currentP3State;
        Serial.printf("[P3] Fizyczny: %s\n", currentP3State ? "ODWROCONE" : "NORMALNE");
        if (currentMode == MODE_WORKING) updateGuns();
    }

    // ═══════════════════════════════════════════════════════════════
    // 5. SAFETY - PERIODIC CHECKS
    // ═══════════════════════════════════════════════════════════════

    static unsigned long lastSafetyCheck = 0;
    if (millis() - lastSafetyCheck > SAFETY_CHECK_INTERVAL_MS) {
        lastSafetyCheck = millis();

        if (!checkHeartbeat()) setStatusLed(false, true, false);

        if (deadmanActive && currentMode == MODE_WORKING) checkDeadman();

        if (currentMode == MODE_WORKING || currentMode == MODE_MEASURING ||
            currentMode == MODE_CALIBRATING) {
            checkEncoderHealth();
        }

        if (currentMode == MODE_WORKING && currentSpeed > 0.1f) validateSpeed();

        // Drift kalibracji co 60s
        static unsigned long lastCalibCheck = 0;
        if (millis() - lastCalibCheck > 60000) {
            lastCalibCheck = millis();
            checkCalibrationDrift();
        }

        // Watchdog
        if (!checkWatchdog()) {
            logError(ERR_WATCHDOG_TIMEOUT, "CRITICAL: Watchdog - restart!");
            ESP.restart();
        }

        // Status LED
        if (currentMode == MODE_WORKING) {
            static bool ledBlink = false;
            ledBlink = !ledBlink;
            if (!speedSufficient) {
                setStatusLed(false, false, ledBlink);  // Żółty blink = za wolno
            } else {
                setStatusLed(ledBlink, false, false);   // Zielony blink = maluje
            }
        } else if (currentMode == MODE_IDLE && !emergencyStopActive) {
            setStatusLed(true, false, false);
        }
    }

    delay(10);
}
