/**
 * @file reports.cpp
 * @brief Implementacja raportów pracy - Trassar-Painter v6.0.0
 *
 * NAPRAWIONE w v6.0.0:
 * - getReportsListJSON() zwraca pole "count" (wymagane przez panel WWW)
 * - exportReportCSV() - prawdziwy parser zamiast placeholder
 * - stopReport() obsługuje pracę przez północ
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <RTClib.h>
#include "reports.h"
#include "config.h"
#include "state.h"
#include "patterns.h"

// Zewnętrzne
extern RTC_DS1307 rtc;

// ============================================================================
// ROZPOCZĘCIE RAPORTU
// ============================================================================

void startReport() {
    if (reportActive) return;

    DateTime now = rtc.now();

    memset(&currentReport, 0, sizeof(WorkReport));
    memset(distancePerPattern, 0, sizeof(distancePerPattern));

    snprintf(currentReport.startDate, sizeof(currentReport.startDate),
             "%04d-%02d-%02d", now.year(), now.month(), now.day());
    snprintf(currentReport.startTime, sizeof(currentReport.startTime),
             "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    reportActive = true;

    Serial.println("[RAPORT] Raport pracy rozpoczety");
    Serial.printf("[RAPORT] Data: %s, Czas: %s\n",
                  currentReport.startDate, currentReport.startTime);
}

// ============================================================================
// AKTUALIZACJA DYSTANSU PER WZORZEC
// ============================================================================

void updatePatternDistance(float deltaDistance) {
    if (!reportActive) return;
    if (currentMode != MODE_WORKING) return;
    if (currentPattern >= 0 && currentPattern < PATTERN_COUNT) {
        distancePerPattern[currentPattern] += deltaDistance;
    }
}

// ============================================================================
// OBLICZANIE POWIERZCHNI
// ============================================================================

float calculateArea(int patternIndex, float distance) {
    if (patternIndex < 0 || patternIndex >= PATTERN_COUNT) return 0.0f;
    float widthMeters = patterns[patternIndex].width / 100.0f;
    return distance * widthMeters;
}

// ============================================================================
// ZAKOŃCZENIE I ZAPIS RAPORTU
// ============================================================================

bool stopReport() {
    if (!reportActive) {
        Serial.println("[RAPORT] Raport nie byl aktywny");
        return false;
    }

    DateTime now = rtc.now();

    snprintf(currentReport.endTime, sizeof(currentReport.endTime),
             "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    // Oblicz czas trwania - NAPRAWIONE: obsługa pracy przez północ
    int startH, startM, startS, endH, endM, endS;
    if (sscanf(currentReport.startTime, "%d:%d:%d", &startH, &startM, &startS) != 3 ||
        sscanf(currentReport.endTime, "%d:%d:%d", &endH, &endM, &endS) != 3) {
        Serial.println("[RAPORT] Blad parsowania czasu");
        reportActive = false;
        return false;
    }

    long startSeconds = startH * 3600L + startM * 60L + startS;
    long endSeconds = endH * 3600L + endM * 60L + endS;

    // Obsługa pracy przez północ (np. 23:00 -> 01:00)
    if (endSeconds < startSeconds) {
        endSeconds += 86400L;  // +24h
    }
    currentReport.workDuration = (unsigned long)(endSeconds - startSeconds);

    // Oblicz powierzchnię
    currentReport.totalArea = 0.0f;
    for (int i = 0; i < PATTERN_COUNT; i++) {
        currentReport.areaPerPattern[i] = calculateArea(i, distancePerPattern[i]);
        currentReport.totalArea += currentReport.areaPerPattern[i];
    }

    // Sprawdź wolne miejsce
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    size_t freeBytes = totalBytes - usedBytes;

    if (freeBytes < MIN_FREE_SPACE_BYTES) {
        Serial.printf("[RAPORT] Brak miejsca w LittleFS! Wolne: %u bytes\n", (unsigned)freeBytes);
        reportActive = false;
        return false;
    }

    // Zapisz raport
    String filename = "/report_" + String(reportCount) + ".txt";
    File file = LittleFS.open(filename, "w");

    if (!file) {
        Serial.printf("[RAPORT] Nie mozna otworzyc: %s\n", filename.c_str());
        reportActive = false;
        return false;
    }

    bool ok = true;
    ok &= (file.printf("RAPORT PRACY - TRASSAR PAINTER v%s\n", FIRMWARE_VERSION) > 0);
    ok &= (file.printf("=====================================\n\n") > 0);
    ok &= (file.printf("Data: %s\n", currentReport.startDate) > 0);
    ok &= (file.printf("Rozpoczecie: %s\n", currentReport.startTime) > 0);
    ok &= (file.printf("Zakonczenie: %s\n", currentReport.endTime) > 0);
    ok &= (file.printf("Czas pracy: %lu s (%lu min)\n\n",
                currentReport.workDuration, currentReport.workDuration / 60) > 0);

    ok &= (file.printf("POWIERZCHNIA WYMALOWANA [m2]:\n") > 0);
    ok &= (file.printf("-----------------------------\n") > 0);

    for (int i = 0; i < PATTERN_COUNT; i++) {
        if (currentReport.areaPerPattern[i] > 0.01f) {
            ok &= (file.printf("%s (%s): %.2f m2 (dystans: %.2f m)\n",
                        patterns[i].name, patterns[i].desc,
                        currentReport.areaPerPattern[i],
                        distancePerPattern[i]) > 0);
        }
    }

    ok &= (file.printf("\n-----------------------------\n") > 0);
    ok &= (file.printf("SUMA: %.2f m2\n", currentReport.totalArea) > 0);
    ok &= (file.printf("=============================\n") > 0);

    file.close();

    if (ok) {
        Serial.printf("[RAPORT] Zapisany: %s (%.2f m2)\n",
                      filename.c_str(), currentReport.totalArea);
        reportCount++;
        reportActive = false;
        return true;
    } else {
        Serial.println("[RAPORT] Blad zapisu - usuwam czesciowy plik");
        LittleFS.remove(filename);
        reportActive = false;
        return false;
    }
}

// ============================================================================
// LISTA RAPORTÓW (JSON)
// ============================================================================
// NAPRAWIONE v6.0.0: Dodane pole "count" wymagane przez panel WWW

String getReportsListJSON() {
    String json;
    json.reserve(512);
    json = "{\"count\":";
    json += String(reportCount);
    json += ",\"reports\":[";

    bool first = true;
    for (int i = 0; i < reportCount; i++) {
        String filename = "/report_" + String(i) + ".txt";
        if (LittleFS.exists(filename)) {
            if (!first) json += ",";
            first = false;
            json += "{\"id\":";
            json += String(i);
            json += ",\"filename\":\"";
            json += filename;
            json += "\"}";
        }
    }

    json += "]}";
    return json;
}

// ============================================================================
// TREŚĆ RAPORTU
// ============================================================================

String getReportContent(int reportId) {
    if (reportId < 0 || reportId >= reportCount) {
        return "Raport nie istnieje";
    }

    String filename = "/report_" + String(reportId) + ".txt";

    if (!LittleFS.exists(filename)) {
        return "Raport nie istnieje";
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        return "Blad odczytu raportu";
    }

    String content;
    content.reserve(file.size());
    while (file.available()) {
        content += char(file.read());
    }
    file.close();

    return content;
}

// ============================================================================
// EKSPORT CSV
// ============================================================================
// NAPRAWIONE v6.0.0: Prawdziwy parser zamiast hardkodowanego placeholder

String exportReportCSV(int reportId) {
    if (reportId < 0 || reportId >= reportCount) {
        return "ERROR: Raport nie istnieje";
    }

    String filename = "/report_" + String(reportId) + ".txt";
    if (!LittleFS.exists(filename)) {
        return "ERROR: Raport nie istnieje";
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        return "ERROR: Nie mozna otworzyc raportu";
    }

    // Parsuj plik tekstowy
    String content;
    content.reserve(file.size());
    while (file.available()) {
        content += char(file.read());
    }
    file.close();

    // Wyciągnij dane z formatu tekstowego
    String csvDate = "", csvStart = "", csvEnd = "", csvDuration = "";

    // Parsuj linie
    int lineStart = 0;
    while (lineStart < (int)content.length()) {
        int lineEnd = content.indexOf('\n', lineStart);
        if (lineEnd == -1) lineEnd = content.length();

        String line = content.substring(lineStart, lineEnd);
        line.trim();

        if (line.startsWith("Data: ")) {
            csvDate = line.substring(6);
        } else if (line.startsWith("Rozpoczecie: ")) {
            csvStart = line.substring(13);
        } else if (line.startsWith("Zakonczenie: ")) {
            csvEnd = line.substring(13);
        } else if (line.startsWith("Czas pracy: ")) {
            // "Czas pracy: 5400 s (90 min)"
            int spaceIdx = line.indexOf(' ', 12);
            if (spaceIdx > 0) {
                csvDuration = line.substring(12, spaceIdx);
            }
        }

        lineStart = lineEnd + 1;
    }

    // Buduj CSV
    String csv;
    csv.reserve(1024);
    csv = "Data,Rozpoczecie,Zakonczenie,Czas_pracy_s,Wzorzec,Opis,Powierzchnia_m2,Dystans_m\n";

    // Parsuj wzorce
    lineStart = 0;
    bool inPatterns = false;
    while (lineStart < (int)content.length()) {
        int lineEnd = content.indexOf('\n', lineStart);
        if (lineEnd == -1) lineEnd = content.length();

        String line = content.substring(lineStart, lineEnd);
        line.trim();

        if (line == "POWIERZCHNIA WYMALOWANA [m2]:") {
            inPatterns = true;
            lineStart = lineEnd + 1;
            continue;
        }

        if (line.startsWith("SUMA:") || line.startsWith("====")) {
            inPatterns = false;
        }

        if (inPatterns && line.startsWith("P-")) {
            // Parsuj: "P-2a (Ciagla waska): 120.50 m2 (dystans: 100.00 m)"
            int parenOpen = line.indexOf('(');
            int parenClose = line.indexOf(')');
            int colonIdx = line.indexOf(':', parenClose);
            int m2Idx = line.indexOf("m2");
            int dystIdx = line.indexOf("dystans:");

            if (parenOpen > 0 && parenClose > parenOpen && colonIdx > 0 && m2Idx > 0) {
                String patName = line.substring(0, parenOpen);
                patName.trim();
                String patDesc = line.substring(parenOpen + 1, parenClose);
                patDesc.trim();
                String area = line.substring(colonIdx + 1, m2Idx);
                area.trim();
                String dist = "0";
                if (dystIdx > 0) {
                    int mIdx = line.indexOf("m", dystIdx + 9);
                    if (mIdx > 0) {
                        dist = line.substring(dystIdx + 9, mIdx);
                        dist.trim();
                    }
                }

                csv += csvDate + ",";
                csv += csvStart + ",";
                csv += csvEnd + ",";
                csv += csvDuration + ",";
                csv += patName + ",";
                csv += patDesc + ",";
                csv += area + ",";
                csv += dist + "\n";
            }
        }

        lineStart = lineEnd + 1;
    }

    // Jeśli brak wzorców - dodaj wiersz podsumowujący
    if (csv.indexOf("P-") == -1) {
        csv += csvDate + "," + csvStart + "," + csvEnd + "," + csvDuration + ",-,-,0,0\n";
    }

    return csv;
}
