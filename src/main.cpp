/**
 * @file main.cpp
 * @brief Trassar-Painter v6.0.0 - MODULAR PRODUCTION EDITION
 *
 * ═══════════════════════════════════════════════════════════════════
 * KOMPUTER MALOWARKI DROGOWEJ - WERSJA MODULARNA PRODUKCYJNA
 * ═══════════════════════════════════════════════════════════════════
 *
 * Plik główny zawiera TYLKO:
 * - Obiekty globalne (TFT, WebServer, RTC, NTP, Preferences)
 * - ISR przycisków (btnStartISR, btnStopISR, btnPauseISR, emergencyStopISR)
 * - Funkcje sterujące (startSystem, stopSystem, pauseSystem, etc.)
 * - setup() i loop()
 *
 * Cała logika jest w modułach:
 * - pins.h      → definicje pinów GPIO (jedyne źródło prawdy)
 * - config.h    → parametry systemu, WiFi, timing
 * - types.h     → struktury, enumy
 * - patterns    → wzorce malowania
 * - state       → zmienne globalne
 * - safety      → system bezpieczeństwa
 * - encoder     → enkoder pomiarowy + kalibracja
 * - reports     → raporty pracy + CSV export
 * - rtc_ntp     → zegar RTC + synchronizacja NTP
 * - buzzer_leds → buzzer + LEDy statusu
 * - display     → wyświetlacz TFT ILI9341
 * - web_panel   → panel WWW + REST API
 *
 * @author Trassar251
 * @date 2026-02-02
 * @version 6.0.0
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <RTClib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
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

// ============================================================================
// OBIEKTY GLOBALNE
// ============================================================================

TFT_eSPI tft = TFT_eSPI();
WebServer server(80);
Preferences prefs;
RTC_DS1307 rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER_PRIMARY, NTP_UTC_OFFSET, NTP_UPDATE_INTERVAL);

// ============================================================================
// ISR PRZYCISKÓW
// ============================================================================

void IRAM_ATTR btnStartISR() {
    static unsigned long lastPress = 0;
    unsigned long now = millis();
    if (now - lastPress > BUTTON_DEBOUNCE_MS) {
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_btnStart = true;
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

void IRAM_ATTR btnPauseISR() {
    static unsigned long lastPress = 0;
    unsigned long now = millis();
    if (now - lastPress > BUTTON_DEBOUNCE_MS) {
        portENTER_CRITICAL_ISR(&isr_mux);
        flag_btnPause = true;
        portEXIT_CRITICAL_ISR(&isr_mux);
        lastPress = now;
    }
}

void IRAM_ATTR emergencyStopISR() {
    portENTER_CRITICAL_ISR(&isr_mux);
    flag_emergencyStop = true;
    portEXIT_CRITICAL_ISR(&isr_mux);
}

// ============================================================================
// FUNKCJE STERUJĄCE
// ============================================================================

/**
 * Ustaw przekaźnik (LOW = OFF, HIGH = ON)
 */
void setRelay(int relayPin, bool state) {
    digitalWrite(relayPin, state ? HIGH : LOW);
}

/**
 * Aktualizuj pistolety według wzorca, trybu i selektora P3
 */
void updateGuns() {
    if (currentMode == MODE_WORKING) {
        bool reverseP3 = selectorP3Physical || selectorP3Virtual;

        for (int i = 0; i < GUN_COUNT; i++) {
            gunsActive[i] = patterns[currentPattern].guns[i];
        }

        // Selektor P3: odwraca P1 <-> P3 dla wzorców podwójnych
        if ((currentPattern == 7 || currentPattern == 8 || currentPattern == 9) && reverseP3) {
            bool temp = gunsActive[0];
            gunsActive[0] = gunsActive[2];
            gunsActive[2] = temp;
        }

        // Start od przerwy - pistolety OFF do przejechania gapLength
        float gapDistance = patterns[currentPattern].gapLength;
        if (startFromGap && gapDistance > 0 && gapTraveled < gapDistance) {
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

    // Fizycznie ustaw przekaźniki - UŻYWAMY RELAY_PINS[]!
    for (int i = 0; i < RELAY_COUNT; i++) {
        setRelay(RELAY_PINS[i], gunsActive[i]);
    }
}

/**
 * START - rozpocznij malowanie
 */
void startSystem() {
    if (currentMode == MODE_IDLE || currentMode == MODE_PAUSED) {
        bool wasIdle = (currentMode == MODE_IDLE);

        currentMode = MODE_WORKING;
        workStartTime = millis();
        encoderPulses = 0;
        distanceTraveled = 0.0f;
        gapTraveled = 0.0f;
        updateGuns();

        if (wasIdle) {
            startReport();
        }

        Serial.printf("[START] Malowanie: %s (%s)\n",
                      patterns[currentPattern].name, patterns[currentPattern].desc);
        if (startFromGap) {
            Serial.printf("[START] Start od przerwy: %.1f m\n",
                          patterns[currentPattern].gapLength);
        }
    }
}

/**
 * START POMIARU - dystans bez malowania
 */
void startMeasuring() {
    currentMode = MODE_MEASURING;
    encoderPulses = 0;
    distanceTraveled = 0.0f;
    updateGuns();
    Serial.println("[START] Tryb pomiaru - pistolety OFF");
}

/**
 * START SERWISU - test pistoletów
 */
void startService() {
    currentMode = MODE_SERVICE;
    serviceTestPattern = -1;
    updateGuns();
    Serial.println("[SERVICE] Tryb serwisowy - test pistoletow");
}

/**
 * WEJŚCIE DO MENU
 */
void enterMenu() {
    currentMode = MODE_MENU;
    menuIndex = 0;
    Serial.println("[MENU] Wejscie do menu");
}

/**
 * STOP - zatrzymanie systemu
 */
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
    updateGuns();

    Serial.printf("[STOP] System zatrzymany (przejechano: %.2f m)\n", distanceTraveled);
}

/**
 * PAUSE/RESUME
 */
void pauseSystem() {
    if (currentMode == MODE_WORKING) {
        totalWorkTime += (millis() - workStartTime);
        currentMode = MODE_PAUSED;
        updateGuns();
        Serial.println("[PAUSE]");
    } else if (currentMode == MODE_PAUSED) {
        currentMode = MODE_WORKING;
        workStartTime = millis();
        updateGuns();
        Serial.println("[RESUME]");
    }
}

/**
 * Zmiana wzorca
 */
void changePattern(int newPattern) {
    if (newPattern >= 0 && newPattern < PATTERN_COUNT) {
        currentPattern = newPattern;
        patternChangeCount++;
        Serial.printf("[PATTERN] Zmiana: %s (%s)\n",
                      patterns[currentPattern].name, patterns[currentPattern].desc);
        if (currentMode == MODE_WORKING) {
            updateGuns();
        }
    }
}

/**
 * Odczyt joysticka i nawigacja menu
 */
void updateJoystick() {
    joyX = analogRead(JOY_VRX);
    joyY = analogRead(JOY_VRY);
    joySW = digitalRead(JOY_SW) == LOW;

    if (currentMode == MODE_MENU) {
        static unsigned long lastJoyMove = 0;
        static bool lastJoySW = false;

        if (millis() - lastJoyMove > JOY_MOVE_DELAY_MS) {
            if (joyY < JOY_THRESHOLD_LOW) {
                menuIndex--;
                if (menuIndex < 0) menuIndex = menuItemsCount - 1;
                Serial.printf("[MENU] UP -> %d\n", menuIndex);
                lastJoyMove = millis();
            } else if (joyY > JOY_THRESHOLD_HIGH) {
                menuIndex++;
                if (menuIndex >= menuItemsCount) menuIndex = 0;
                Serial.printf("[MENU] DOWN -> %d\n", menuIndex);
                lastJoyMove = millis();
            }
        }

        if (joySW && !lastJoySW) {
            Serial.printf("[MENU] Wybrano: %s\n", menuItems[menuIndex]);
            if (menuIndex == 0) startCalibration();
            else if (menuIndex == 1) {
                Serial.println("[MENU] Raporty -> panel WWW 192.168.4.1");
                currentMode = MODE_IDLE;
            } else if (menuIndex == 2) {
                Serial.println("[MENU] OTA aktywne - hostname: " OTA_HOSTNAME);
                currentMode = MODE_IDLE;
            } else if (menuIndex == 6) startService();
            else currentMode = MODE_IDLE;
        }

        lastJoySW = joySW;
    }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n================================================");
    Serial.printf("  TRASSAR PAINTER v%s\n", FIRMWARE_VERSION);
    Serial.printf("  %s\n", FIRMWARE_SUBTITLE);
    Serial.println("  E-STOP + Watchdog + Fail-safe + Raporty");
    Serial.println("  15 wzorcow + Auto-kalibracja + Panel WWW");
    Serial.println("================================================\n");

    // LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("[FS] Blad montowania LittleFS");
    } else {
        Serial.println("[FS] LittleFS zamontowany");
        reportCount = 0;
        while (LittleFS.exists("/report_" + String(reportCount) + ".txt")) {
            reportCount++;
        }
        Serial.printf("[FS] Znaleziono %d raportow\n", reportCount);
    }

    // TFT
    tftInit();

    // Preferences - kalibracja
    prefs.begin(NVS_NAMESPACE, false);
    encoderCalibration = prefs.getFloat(NVS_KEY_CALIBRATION, DEFAULT_CALIBRATION);
    Serial.printf("[CALIB] Enkoder: %.1f imp/m\n", encoderCalibration);

    // GPIO - Przekaźniki
    for (int i = 0; i < RELAY_COUNT; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], LOW);
    }
    Serial.println("[GPIO] Przekazniki OK");

    // GPIO - Przyciski
    pinMode(BTN_START, INPUT_PULLUP);
    pinMode(BTN_STOP, INPUT_PULLUP);
    pinMode(BTN_PAUSE, INPUT_PULLUP);
    Serial.println("[GPIO] Przyciski OK");

    // GPIO - Selektor P3
    pinMode(SEL_P3, INPUT_PULLUP);

    // GPIO - Joystick
    pinMode(JOY_SW, INPUT_PULLUP);

    // GPIO - Enkoder
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

    // Przerwania - Przyciski
    attachInterrupt(digitalPinToInterrupt(BTN_START), btnStartISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_STOP), btnStopISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_PAUSE), btnPauseISR, FALLING);

    // Przerwanie - E-STOP (najwyższy priorytet!)
    attachInterrupt(digitalPinToInterrupt(BTN_EMERGENCY_STOP), emergencyStopISR, FALLING);
    Serial.println("[INT] Przerwania skonfigurowane");

    // Zapisz początkową kalibrację (drift detection)
    initialCalibration = encoderCalibration;

    // Self-test
    performSelfTest();

    // Inicjalizuj timery safety
    lastWatchdogReset = millis();
    lastHeartbeat = millis();
    lastDeadmanConfirm = millis();
    lastEncoderChange = millis();

    // WiFi AP+STA
    Serial.println("\n[WiFi] Tryb AP+STA");
    WiFi.mode(WIFI_AP_STA);

    // STA - połączenie z WiFi użytkownika (NTP)
    Serial.printf("[WiFi STA] Laczenie z %s...\n", WIFI_STA_SSID);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi STA] Polaczono! IP: %s\n", WiFi.localIP().toString().c_str());
        syncNTP();
    } else {
        Serial.println("\n[WiFi STA] Nie udalo sie polaczyc");
        Serial.println("[WiFi STA] RTC uzyje poprzednio zapisanego czasu");
    }

    // AP - panel WWW (zawsze dostępny)
    if (WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD)) {
        Serial.printf("[WiFi AP] SSID: %s, IP: %s\n",
                      WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());

        // OTA
        ArduinoOTA.setHostname(OTA_HOSTNAME);
        ArduinoOTA.setPassword(OTA_PASSWORD);

        ArduinoOTA.onStart([]() {
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            Serial.println("[OTA] Start: " + type);
            LittleFS.end();
        });
        ArduinoOTA.onEnd([]() { Serial.println("\n[OTA] Zakonczone!"); });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[OTA] %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("[OTA] Blad[%u]\n", error);
        });
        ArduinoOTA.begin();
        Serial.printf("[OTA] Hostname: %s\n", OTA_HOSTNAME);

        // Web Server
        setupWebServer();

        Serial.println("\n================================================");
        Serial.println("  PANEL: http://192.168.4.1");
        Serial.printf("  OTA: %s (%s)\n", OTA_HOSTNAME, OTA_PASSWORD);
        Serial.println("================================================\n");
    } else {
        Serial.println("[WiFi AP] BLAD uruchamiania!");
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

    // E-STOP
    if (flag_emergencyStop) {
        portENTER_CRITICAL(&isr_mux);
        flag_emergencyStop = false;
        portEXIT_CRITICAL(&isr_mux);
        activateEmergencyStop();
    }

    // Gdy E-STOP aktywny - blokuj wszystko poza resetem
    if (emergencyStopActive) {
        buzzerUpdate();
        updateStatusLeds();

        if (digitalRead(BTN_EMERGENCY_STOP) == HIGH && flag_btnStart) {
            portENTER_CRITICAL(&isr_mux);
            flag_btnStart = false;
            portEXIT_CRITICAL(&isr_mux);
            resetEmergencyStop();
        }

        delay(50);
        return;
    }

    // Watchdog + Heartbeat
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

    if (flag_btnStart) {
        portENTER_CRITICAL(&isr_mux);
        flag_btnStart = false;
        portEXIT_CRITICAL(&isr_mux);
        startSystem();
        confirmDeadman();
    }

    if (flag_btnStop) {
        portENTER_CRITICAL(&isr_mux);
        flag_btnStop = false;
        portEXIT_CRITICAL(&isr_mux);
        stopSystem();
    }

    if (flag_btnPause) {
        portENTER_CRITICAL(&isr_mux);
        flag_btnPause = false;
        portEXIT_CRITICAL(&isr_mux);
        pauseSystem();
        confirmDeadman();
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

    // TFT update (odkomentuj gdy podłączysz wyświetlacz)
    /*
    if (millis() - lastTFTUpdate > TFT_UPDATE_INTERVAL_MS) {
        if (currentMode != MODE_MENU) {
            tftDrawStatus();
        }
        lastTFTUpdate = millis();
    }
    */

    // ═══════════════════════════════════════════════════════════════
    // 5. SAFETY - PERIODIC CHECKS (co 1s)
    // ═══════════════════════════════════════════════════════════════

    static unsigned long lastSafetyCheck = 0;
    if (millis() - lastSafetyCheck > SAFETY_CHECK_INTERVAL_MS) {
        lastSafetyCheck = millis();

        if (!checkHeartbeat()) {
            setStatusLed(false, true, false);
        }

        if (deadmanActive && currentMode == MODE_WORKING) {
            checkDeadman();
        }

        if (currentMode == MODE_WORKING || currentMode == MODE_MEASURING ||
            currentMode == MODE_CALIBRATING) {
            checkEncoderHealth();
        }

        if (currentMode == MODE_WORKING && currentSpeed > 0.1f) {
            validateSpeed();
        }

        // Drift kalibracji (co 60s)
        static unsigned long lastCalibCheck = 0;
        if (millis() - lastCalibCheck > CALIB_CHECK_INTERVAL_MS) {
            lastCalibCheck = millis();
            checkCalibrationDrift();
        }

        // Watchdog
        if (!checkWatchdog()) {
            logError(ERR_WATCHDOG_TIMEOUT, "CRITICAL: Watchdog timeout - restart!");
            ESP.restart();
        }

        // Status LED
        if (currentMode == MODE_WORKING) {
            static bool ledBlink = false;
            ledBlink = !ledBlink;
            setStatusLed(ledBlink, false, false);
        } else if (currentMode == MODE_IDLE && !emergencyStopActive) {
            setStatusLed(true, false, false);
        }
    }

    delay(10);
}
