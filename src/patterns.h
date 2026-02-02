/**
 * @file patterns.h
 * @brief Definicje wzorców malowania drogowego - Trassar-Painter v6.0.0
 *
 * 15 wzorców zgodnych z polskimi normami (P-1a do P-7d).
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef PATTERNS_H
#define PATTERNS_H

#include "types.h"

// Tablica 15 wzorców (definicja w patterns.cpp)
extern const PatternInfo patterns[PATTERN_COUNT];

/**
 * Pobierz nazwę trybu jako string
 * @param mode Tryb systemu
 * @return Nazwa trybu (const char*)
 */
const char* getModeName(SystemMode mode);

/**
 * Pobierz nazwę trybu jako string dla JSON API (lowercase)
 * @param mode Tryb systemu
 * @return Nazwa trybu lowercase
 */
const char* getModeNameJSON(SystemMode mode);

#endif // PATTERNS_H
