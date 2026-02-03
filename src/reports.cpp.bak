/**
 * @file reports.cpp
 * @brief Implementacja raportów pracy - Trassar-Painter v7.0.0
 *
 * ZMIANY v7.0.0:
 * - Zapis raportów na kartę SD (fallback na LittleFS gdy brak SD)
 * - Dodano deleteReport() i deleteAllReports()
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <SD.h>
#include <RTClib.h>
#include "reports.h"
#include "config.h"
#include "state.h"
#include "patterns.h"

extern RTC_DS1307 rtc;

// Helper: wybierz system plików
static fs::FS& getReportFS() {
    if (sdCardAvailable) return SD;
    return LittleFS;
}

static String getReportPath(int id) {
    if (sdCardAvailable) {
        return String(SD_REPORTS_DIR) + "/report_" + String(id) + ".txt";
    }
    return "/report_" + String(id) + ".txt";
}

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
    Serial.printf("[RAPORT] Rozpoczety: %s %s\n", currentReport.startDate, currentReport.startTime);
}

// ============================================================================
// AKTUALIZACJA DYSTANSU
// ============================================================================

void updatePatternDistance(float deltaDistance) {
    if (!reportActive || currentMode != MODE_WORKING) return;
    if (currentPattern >= 0 && currentPattern < PATTERN_COUNT) {
        distancePerPattern[currentPattern] += deltaDistance;
    }
}

float calculateArea(int patternIndex, float distance) {
    if (patternIndex < 0 || patternIndex >= PATTERN_COUNT) return 0.0f;
    return distance * (patterns[patternIndex].width / 100.0f);
}

// ============================================================================
// ZAKOŃCZENIE I ZAPIS RAPORTU
// ============================================================================

bool stopReport() {
    if (!reportActive) return false;

    DateTime now = rtc.now();
    snprintf(currentReport.endTime, sizeof(currentReport.endTime),
             "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    // Czas trwania z obsługą północy
    int startH, startM, startS, endH, endM, endS;
    if (sscanf(currentReport.startTime, "%d:%d:%d", &startH, &startM, &startS) != 3 ||
        sscanf(currentReport.endTime, "%d:%d:%d", &endH, &endM, &endS) != 3) {
        reportActive = false;
        return false;
    }

    long startSec = startH * 3600L + startM * 60L + startS;
    long endSec = endH * 3600L + endM * 60L + endS;
    if (endSec < startSec) endSec += 86400L;
    currentReport.workDuration = (unsigned long)(endSec - startSec);

    // Oblicz powierzchnię
    currentReport.totalArea = 0.0f;
    for (int i = 0; i < PATTERN_COUNT; i++) {
        currentReport.areaPerPattern[i] = calculateArea(i, distancePerPattern[i]);
        currentReport.totalArea += currentReport.areaPerPattern[i];
    }

    // Zapisz raport
    fs::FS& fs = getReportFS();
    String filename = getReportPath(reportCount);

    // Utwórz katalog na SD jeśli potrzeba
    if (sdCardAvailable) {
        if (!SD.exists(SD_REPORTS_DIR)) {
            SD.mkdir(SD_REPORTS_DIR);
        }
    }

    File file = fs.open(filename, "w");
    if (!file) {
        Serial.printf("[RAPORT] Nie mozna otworzyc: %s\n", filename.c_str());
        reportActive = false;
        return false;
    }

    file.printf("RAPORT PRACY - TRASSAR PAINTER v%s\n", FIRMWARE_VERSION);
    file.printf("=====================================\n\n");
    file.printf("Data: %s\n", currentReport.startDate);
    file.printf("Rozpoczecie: %s\n", currentReport.startTime);
    file.printf("Zakonczenie: %s\n", currentReport.endTime);
    file.printf("Czas pracy: %lu s (%lu min)\n\n",
                currentReport.workDuration, currentReport.workDuration / 60);

    file.printf("POWIERZCHNIA WYMALOWANA [m2]:\n");
    file.printf("-----------------------------\n");

    for (int i = 0; i < PATTERN_COUNT; i++) {
        if (currentReport.areaPerPattern[i] > 0.01f) {
            file.printf("%s (%s): %.2f m2 (dystans: %.2f m)\n",
                        patterns[i].name, patterns[i].desc,
                        currentReport.areaPerPattern[i], distancePerPattern[i]);
        }
    }

    file.printf("\n-----------------------------\n");
    file.printf("SUMA: %.2f m2\n", currentReport.totalArea);
    file.printf("=============================\n");
    file.close();

    Serial.printf("[RAPORT] Zapisany: %s (%.2f m2) [%s]\n",
                  filename.c_str(), currentReport.totalArea,
                  sdCardAvailable ? "SD" : "LittleFS");
    reportCount++;
    reportActive = false;
    return true;
}

// ============================================================================
// LISTA RAPORTÓW (JSON)
// ============================================================================

String getReportsListJSON() {
    String json;
    json.reserve(512);
    json = "{\"count\":";
    json += String(reportCount);
    json += ",\"storage\":\"";
    json += (sdCardAvailable ? "SD" : "LittleFS");
    json += "\",\"reports\":[";

    bool first = true;
    fs::FS& fs = getReportFS();
    for (int i = 0; i < reportCount; i++) {
        String filename = getReportPath(i);
        if (fs.exists(filename)) {
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
    if (reportId < 0 || reportId >= reportCount) return "Raport nie istnieje";

    fs::FS& fs = getReportFS();
    String filename = getReportPath(reportId);

    if (!fs.exists(filename)) return "Raport nie istnieje";

    File file = fs.open(filename, "r");
    if (!file) return "Blad odczytu raportu";

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

String exportReportCSV(int reportId) {
    if (reportId < 0 || reportId >= reportCount) return "ERROR: Raport nie istnieje";

    String content = getReportContent(reportId);
    if (content.startsWith("ERROR") || content.startsWith("Raport") || content.startsWith("Blad")) {
        return content;
    }

    String csvDate = "", csvStart = "", csvEnd = "", csvDuration = "";

    int lineStart = 0;
    while (lineStart < (int)content.length()) {
        int lineEnd = content.indexOf('\n', lineStart);
        if (lineEnd == -1) lineEnd = content.length();
        String line = content.substring(lineStart, lineEnd);
        line.trim();

        if (line.startsWith("Data: ")) csvDate = line.substring(6);
        else if (line.startsWith("Rozpoczecie: ")) csvStart = line.substring(13);
        else if (line.startsWith("Zakonczenie: ")) csvEnd = line.substring(13);
        else if (line.startsWith("Czas pracy: ")) {
            int spaceIdx = line.indexOf(' ', 12);
            if (spaceIdx > 0) csvDuration = line.substring(12, spaceIdx);
        }
        lineStart = lineEnd + 1;
    }

    String csv;
    csv.reserve(1024);
    csv = "Data,Rozpoczecie,Zakonczenie,Czas_pracy_s,Wzorzec,Opis,Powierzchnia_m2,Dystans_m\n";

    lineStart = 0;
    bool inPatterns = false;
    while (lineStart < (int)content.length()) {
        int lineEnd = content.indexOf('\n', lineStart);
        if (lineEnd == -1) lineEnd = content.length();
        String line = content.substring(lineStart, lineEnd);
        line.trim();

        if (line == "POWIERZCHNIA WYMALOWANA [m2]:") { inPatterns = true; lineStart = lineEnd + 1; continue; }
        if (line.startsWith("SUMA:") || line.startsWith("====")) inPatterns = false;

        if (inPatterns && line.startsWith("P-")) {
            int parenOpen = line.indexOf('(');
            int parenClose = line.indexOf(')');
            int colonIdx = line.indexOf(':', parenClose);
            int m2Idx = line.indexOf("m2");
            int dystIdx = line.indexOf("dystans:");

            if (parenOpen > 0 && parenClose > parenOpen && colonIdx > 0 && m2Idx > 0) {
                String patName = line.substring(0, parenOpen); patName.trim();
                String patDesc = line.substring(parenOpen + 1, parenClose); patDesc.trim();
                String area = line.substring(colonIdx + 1, m2Idx); area.trim();
                String dist = "0";
                if (dystIdx > 0) {
                    int mIdx = line.indexOf("m", dystIdx + 9);
                    if (mIdx > 0) { dist = line.substring(dystIdx + 9, mIdx); dist.trim(); }
                }
                csv += csvDate + "," + csvStart + "," + csvEnd + "," + csvDuration + ",";
                csv += patName + "," + patDesc + "," + area + "," + dist + "\n";
            }
        }
        lineStart = lineEnd + 1;
    }

    if (csv.indexOf("P-") == -1) {
        csv += csvDate + "," + csvStart + "," + csvEnd + "," + csvDuration + ",-,-,0,0\n";
    }

    return csv;
}

// ============================================================================
// USUWANIE RAPORTÓW (NOWE v7.0.0)
// ============================================================================

bool deleteReport(int reportId) {
    if (reportId < 0 || reportId >= reportCount) return false;
    fs::FS& fs = getReportFS();
    String filename = getReportPath(reportId);
    if (fs.exists(filename)) {
        fs.remove(filename);
        Serial.printf("[RAPORT] Usuniety: %s\n", filename.c_str());
        return true;
    }
    return false;
}

bool deleteAllReports() {
    fs::FS& fs = getReportFS();
    for (int i = 0; i < reportCount; i++) {
        String filename = getReportPath(i);
        if (fs.exists(filename)) {
            fs.remove(filename);
        }
    }
    Serial.printf("[RAPORT] Usunieto %d raportow\n", reportCount);
    reportCount = 0;
    return true;
}
