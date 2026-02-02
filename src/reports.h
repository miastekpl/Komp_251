/**
 * @file reports.h
 * @brief System raportów pracy - Trassar-Painter v6.0.0
 *
 * Automatyczne raportowanie sesji, powierzchnia m², eksport CSV.
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef REPORTS_H
#define REPORTS_H

#include <Arduino.h>

// Rozpocznij nowy raport (przy pierwszym START)
void startReport();

// Aktualizuj dystans dla bieżącego wzorca
void updatePatternDistance(float deltaDistance);

// Oblicz powierzchnię [m²] = dystans × szerokość
float calculateArea(int patternIndex, float distance);

// Zakończ i zapisz raport do LittleFS
bool stopReport();

// Pobierz listę raportów (JSON z polem "count")
String getReportsListJSON();

// Pobierz treść raportu tekstowego
String getReportContent(int reportId);

// Eksport raportu do formatu CSV (NAPRAWIONY w v6.0.0)
String exportReportCSV(int reportId);

#endif // REPORTS_H
