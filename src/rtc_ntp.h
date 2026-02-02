/**
 * @file rtc_ntp.h
 * @brief RTC DS1307 + NTP - Trassar-Painter v6.0.0
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef RTC_NTP_H
#define RTC_NTP_H

#include <Arduino.h>

void initRTC();
bool syncNTP();
String getCurrentDateTime();

#endif // RTC_NTP_H
