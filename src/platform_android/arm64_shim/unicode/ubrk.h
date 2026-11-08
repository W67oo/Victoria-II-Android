#pragma once
// =============================================================================
// arm64_shim/unicode/ubrk.h
// UAX #14 CJK + Kinsoku Shori + Latin BreakIterator for Project Alice
// =============================================================================

#include "utypes.h"

#ifndef UBRK_DONE
#define UBRK_DONE (-1)
#endif

enum UBreakIteratorType : int32_t {
    UBRK_CHARACTER = 0,
    UBRK_WORD      = 1,
    UBRK_LINE      = 2,
    UBRK_SENTENCE  = 3
};

struct UBreakIterator;

extern "C" {
UBreakIterator* ubrk_open(UBreakIteratorType type, const char* locale,
                          const UChar* text, int32_t textLength, UErrorCode* status);
int32_t ubrk_getBinaryRules(UBreakIterator* bi, uint8_t* binaryRules,
                            int32_t rulesCapacity, UErrorCode* status);
UBreakIterator* ubrk_openBinaryRules(const uint8_t* binaryRules, int32_t rulesLength,
                                     const UChar* text, int32_t textLength, UErrorCode* status);
void    ubrk_close(UBreakIterator* bi);
int32_t ubrk_first(UBreakIterator* bi);
int32_t ubrk_next(UBreakIterator* bi);
int32_t ubrk_previous(UBreakIterator* bi);
}
