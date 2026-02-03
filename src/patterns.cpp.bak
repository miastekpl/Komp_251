/**
 * @file patterns.cpp
 * @brief Implementacja wzorców malowania - Trassar-Painter v7.0.0
 */

#include "patterns.h"

const PatternInfo patterns[PATTERN_COUNT] = {
    // idx 0: P-1a - Przerywana długa
    {"P-1a", "Przerywana dluga",       4.0f, 8.0f, 12, {false, true,  false, false, false, false}},
    // idx 1: P-1b - Przerywana krótka
    {"P-1b", "Przerywana krotka",      2.0f, 4.0f, 12, {false, true,  false, false, false, false}},
    // idx 2: P-1c - Wydzielająca
    {"P-1c", "Wydzielajaca",           2.0f, 2.0f, 12, {false, true,  false, false, false, false}},
    // idx 3: P-1d - Prowadząca wąska
    {"P-1d", "Prowadzaca waska",       1.0f, 1.0f, 12, {false, true,  false, false, false, false}},
    // idx 4: P-1e - Prowadząca szeroka
    {"P-1e", "Prowadz. szeroka",       1.0f, 1.0f, 24, {false, false, false, true,  false, false}},
    // idx 5: P-2a - Ciągła wąska (DOMYŚLNY)
    {"P-2a", "Ciagla waska",           0.0f, 0.0f, 12, {false, true,  false, false, false, false}},
    // idx 6: P-2b - Ciągła szeroka
    {"P-2b", "Ciagla szeroka",         0.0f, 0.0f, 24, {false, false, false, true,  false, false}},
    // idx 7: P-3a - Przekraczalna długa (podwójna)
    {"P-3a", "Przekraczalna dl.",      4.0f, 2.0f, 12, {true,  false, true,  false, false, false}},
    // idx 8: P-3b - Przekraczalna krótka (podwójna)
    {"P-3b", "Przekraczalna kr.",      1.0f, 1.0f, 12, {true,  false, true,  false, false, false}},
    // idx 9: P-4 - Podwójna ciągła
    {"P-4",  "Podwojna ciagla",        0.0f, 0.0f, 24, {true,  false, true,  false, false, false}},
    // idx 10: P-6 - Ostrzegawcza
    {"P-6",  "Ostrzegawcza",           4.0f, 2.0f, 12, {false, false, false, false, true,  false}},
    // idx 11: P-7a - Krawędziowa przerywana szeroka
    {"P-7a", "Kraw. przeryw. sz",      1.0f, 1.0f, 24, {false, false, false, false, false, true }},
    // idx 12: P-7b - Krawędziowa ciągła szeroka
    {"P-7b", "Kraw. ciagla sz.",       0.0f, 0.0f, 24, {false, false, false, false, false, true }},
    // idx 13: P-7c - Krawędziowa przerywana wąska
    {"P-7c", "Kraw. przeryw. w.",      1.0f, 1.0f, 12, {false, false, false, false, true,  false}},
    // idx 14: P-7d - Krawędziowa ciągła wąska
    {"P-7d", "Kraw. ciagla w.",        0.0f, 0.0f, 12, {false, false, false, false, true,  false}}
};

const char* getModeName(SystemMode mode) {
    switch (mode) {
        case MODE_IDLE:         return "IDLE";
        case MODE_WORKING:      return "MALOWANIE";
        case MODE_PAUSED:       return "PAUZA";
        case MODE_MEASURING:    return "POMIAR";
        case MODE_MENU:         return "MENU";
        case MODE_SERVICE:      return "SERWIS";
        case MODE_CALIBRATING:  return "KALIBRACJA";
        case MODE_EMERGENCY:    return "AWARYJNY";
        default:                return "UNKNOWN";
    }
}

const char* getModeNameJSON(SystemMode mode) {
    switch (mode) {
        case MODE_IDLE:         return "idle";
        case MODE_WORKING:      return "working";
        case MODE_PAUSED:       return "paused";
        case MODE_MEASURING:    return "measuring";
        case MODE_MENU:         return "menu";
        case MODE_SERVICE:      return "service";
        case MODE_CALIBRATING:  return "calibrating";
        case MODE_EMERGENCY:    return "emergency";
        default:                return "unknown";
    }
}

bool isPatternDashed(int patternIndex) {
    if (patternIndex < 0 || patternIndex >= PATTERN_COUNT) return false;
    return (patterns[patternIndex].lineLength > 0 && patterns[patternIndex].gapLength > 0);
}
