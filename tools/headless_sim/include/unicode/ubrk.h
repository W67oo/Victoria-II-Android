#pragma once
// =============================================================================
// tools/headless_sim/include/unicode/ubrk.h
// Self-contained ICU BreakIterator declarations for headless host build
// =============================================================================

#include "utypes.h"

struct UBreakIterator;

enum UBreakIteratorType {
    UBRK_CHARACTER = 0,
    UBRK_WORD = 1,
    UBRK_LINE = 2,
    UBRK_SENTENCE = 3
};

#define UBRK_DONE ((int32_t)-1)

#ifdef __cplusplus
extern "C" {
#endif

UBreakIterator* ubrk_open(UBreakIteratorType type, const char* locale, const UChar* text, int32_t textLength, UErrorCode* status);
void ubrk_close(UBreakIterator* bi);
void ubrk_setText(UBreakIterator* bi, const UChar* text, int32_t textLength, UErrorCode* status);
int32_t ubrk_first(UBreakIterator* bi);
int32_t ubrk_next(UBreakIterator* bi);
int32_t ubrk_previous(UBreakIterator* bi);
int32_t ubrk_current(const UBreakIterator* bi);
int32_t ubrk_getBinaryRules(UBreakIterator* bi, uint8_t* rules, int32_t rulesCapacity, UErrorCode* status);
UBreakIterator* ubrk_openBinaryRules(const uint8_t* binaryRules, int32_t rulesLength, const UChar* text, int32_t textLength, UErrorCode* status);

#ifdef __cplusplus
}
#endif
