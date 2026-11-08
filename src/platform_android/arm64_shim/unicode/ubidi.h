#pragma once
// =============================================================================
// arm64_shim/unicode/ubidi.h
// UAX #9 BiDi paragraph & visual run segmentation for Project Alice
// =============================================================================

#include "utypes.h"

using UBiDiLevel = uint8_t;

enum UBiDiDirection : int32_t {
    UBIDI_LTR   = 0,
    UBIDI_RTL   = 1,
    UBIDI_MIXED = 2,
    UBIDI_NEUTRAL = 3
};

struct UBiDi;

extern "C" {
UBiDi*         ubidi_open();
void           ubidi_close(UBiDi* pBiDi);
void           ubidi_setPara(UBiDi* pBiDi, const UChar* text, int32_t length,
                             UBiDiLevel paraLevel, UBiDiLevel* embeddingLevels, UErrorCode* pErrorCode);
int32_t        ubidi_countRuns(UBiDi* pBiDi, UErrorCode* pErrorCode);
UBiDiDirection ubidi_getVisualRun(UBiDi* pBiDi, int32_t runIndex,
                                  int32_t* pLogicalStart, int32_t* pLength);
}
