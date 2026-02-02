/**
 * @file reports.h
 * @brief System raportów pracy - Trassar-Painter v7.0.0
 *
 * v7.0.0: Raporty zapisywane na kartę SD (fallback na LittleFS)
 *         Dodano deleteReport() i deleteAllReports()
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef REPORTS_H
#define REPORTS_H

#include <Arduino.h>

void startReport();
void updatePatternDistance(float deltaDistance);
float calculateArea(int patternIndex, float distance);
bool stopReport();
String getReportsListJSON();
String getReportContent(int reportId);
String exportReportCSV(int reportId);
bool deleteReport(int reportId);
bool deleteAllReports();

#endif // REPORTS_H
