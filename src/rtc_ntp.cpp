/**
 * @file rtc_ntp.cpp
 * @brief Implementacja RTC i NTP - Trassar-Painter v7.0.0
 *
 * v7.0.0: Tylko configTime() + getLocalTime() (usunięto NTPClient)
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include "rtc_ntp.h"
#include "pins.h"
#include "config.h"

extern RTC_DS1307 rtc;

void initRTC() {
    Wire.begin(RTC_SDA, RTC_SCL);

    if (!rtc.begin()) {
        Serial.printf("[RTC] Nie znaleziono DS1307! (SDA=%d, SCL=%d)\n", RTC_SDA, RTC_SCL);
        return;
    }

    Serial.println("[RTC] DS1307 znaleziony");

    if (!rtc.isrunning()) {
        Serial.println("[RTC] Zegar nie dziala, czas zostanie ustawiony przez NTP...");
    } else {
        DateTime now = rtc.now();
        Serial.printf("[RTC] Czas: %04d-%02d-%02d %02d:%02d:%02d\n",
                      now.year(), now.month(), now.day(),
                      now.hour(), now.minute(), now.second());
    }
}

bool syncNTP() {
    Serial.println("[NTP] Synchronizacja czasu...");

    configTime(NTP_GMT_OFFSET, NTP_DAYLIGHT_OFFSET, NTP_SERVER);

    struct tm timeinfo;

    for (int i = 0; i < 5; i++) {
        Serial.printf("[NTP] Proba %d/5...\n", i + 1);
        if (getLocalTime(&timeinfo, 2000)) {
            rtc.adjust(DateTime(
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday,
                timeinfo.tm_hour,
                timeinfo.tm_min,
                timeinfo.tm_sec
            ));

            DateTime now = rtc.now();
            Serial.printf("[NTP] Zsynchronizowano: %04d-%02d-%02d %02d:%02d:%02d\n",
                          now.year(), now.month(), now.day(),
                          now.hour(), now.minute(), now.second());
            return true;
        }
        delay(1000);
    }

    Serial.println("[NTP] Nie udalo sie pobrac czasu");
    return false;
}

String getCurrentDateTime() {
    DateTime now = rtc.now();
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    return String(buf);
}
