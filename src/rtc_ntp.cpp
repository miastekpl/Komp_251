/**
 * @file rtc_ntp.cpp
 * @brief Implementacja RTC i NTP - Trassar-Painter v6.0.0
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "rtc_ntp.h"
#include "pins.h"
#include "config.h"

// Zewnętrzne obiekty (z main.cpp)
extern RTC_DS1307 rtc;
extern NTPClient timeClient;

// ============================================================================
// INICJALIZACJA RTC
// ============================================================================

void initRTC() {
    Wire.begin(RTC_SDA, RTC_SCL);

    if (!rtc.begin()) {
        Serial.println("[RTC] Nie znaleziono modulu DS1307!");
        Serial.printf("[RTC] Sprawdz polaczenia I2C (SDA=%d, SCL=%d)\n", RTC_SDA, RTC_SCL);
        return;
    }

    Serial.println("[RTC] DS1307 znaleziony");

    if (!rtc.isrunning()) {
        Serial.println("[RTC] Zegar nie dziala, czas zostanie ustawiony przez NTP...");
    } else {
        DateTime now = rtc.now();
        Serial.printf("[RTC] Aktualny czas: %04d-%02d-%02d %02d:%02d:%02d\n",
                      now.year(), now.month(), now.day(),
                      now.hour(), now.minute(), now.second());
    }
}

// ============================================================================
// SYNCHRONIZACJA NTP
// ============================================================================

bool syncNTP() {
    Serial.println("[NTP] Synchronizacja czasu...");

    configTime(NTP_UTC_OFFSET, NTP_DST_OFFSET, NTP_SERVER_PRIMARY, NTP_SERVER_SECONDARY);

    if (!timeClient.begin()) {
        Serial.println("[NTP] Blad inicjalizacji NTP Client");
        return false;
    }

    bool success = false;
    for (int i = 0; i < NTP_MAX_RETRIES; i++) {
        Serial.printf("[NTP] Proba %d/%d...\n", i + 1, NTP_MAX_RETRIES);

        if (timeClient.update()) {
            success = true;
            break;
        }

        delay(NTP_RETRY_DELAY_MS);
    }

    if (success && timeClient.isTimeSet()) {
        unsigned long epochTime = timeClient.getEpochTime();

        // Sprawdź poprawność czasu (>= 2020)
        if (epochTime < 1577836800UL) {
            Serial.println("[NTP] Otrzymano nieprawidlowy czas");
            return false;
        }

        rtc.adjust(DateTime(epochTime));

        DateTime now = rtc.now();
        Serial.printf("[NTP] Zsynchronizowano: %04d-%02d-%02d %02d:%02d:%02d\n",
                      now.year(), now.month(), now.day(),
                      now.hour(), now.minute(), now.second());

        return true;
    } else {
        Serial.println("[NTP] Nie udalo sie pobrac czasu");
        Serial.println("[NTP] RTC bedzie uzywac poprzednio zapisanego czasu");
        return false;
    }
}

// ============================================================================
// AKTUALNY CZAS
// ============================================================================

String getCurrentDateTime() {
    DateTime now = rtc.now();
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    return String(buf);
}
