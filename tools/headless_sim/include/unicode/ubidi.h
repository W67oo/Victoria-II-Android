#pragma once
// =============================================================================
// tools/headless_sim/include/unicode/ubidi.h
// Self-contained ICU BiDi declarations for headless host build
// =============================================================================

#include "utypes.h"

struct UBiDi;

enum UBiDiDirection {
    UBIDI_LTR = 0,
    UBIDI_RTL = 1,
    UBIDI_MIXED = 2,
    UBIDI_NEUTRAL = 3
};

typedef uint8_t UBiDiLevel;

#ifdef __cplusplus
extern "C" {
#endif

UBiDi* ubidi_open(void);
void ubidi_close(UBiDi* pBiDi);
void ubidi_setPara(UBiDi* pBiDi, const UChar* text, int32_t length, UBiDiLevel paraLevel, UBiDiLevel* embeddingLevels, UErrorCode* pErrorCode);
int32_t ubidi_countRuns(UBiDi* pBiDi, UErrorCode* pErrorCode);
UBiDiDirection ubidi_getVisualRun(UBiDi* pBiDi, int32_t runIndex, int32_t* pLogicalStart, int32_t* pLength);

#ifdef __cplusplus
}
#endif
