/**
 * @file patterns.h
 * @brief Definicje wzorców malowania drogowego - Trassar-Painter v7.0.0
 *
 * @author Trassar251
 * @date 2026-02-02
 */

#ifndef PATTERNS_H
#define PATTERNS_H

#include "types.h"

extern const PatternInfo patterns[PATTERN_COUNT];

const char* getModeName(SystemMode mode);
const char* getModeNameJSON(SystemMode mode);

// v7.0.0: Sprawdza czy wzorzec jest przerywany (ma cykl linia/przerwa)
bool isPatternDashed(int patternIndex);

#endif // PATTERNS_H
