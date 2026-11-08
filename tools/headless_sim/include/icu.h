#pragma once
// =============================================================================
// tools/headless_sim/include/icu.h
// Lightweight ICU C Shim for Headless Host Windows Build
// Eliminates binary icu.lib/icudt dependency on host.
// =============================================================================

#include <cstdint>
#include <cwchar>

#ifndef UErrorCode
typedef int32_t UErrorCode;
#endif

#ifndef UChar
typedef uint16_t UChar;
#endif

#ifndef UBool
typedef int8_t UBool;
#endif

typedef void* UCollator;

#ifndef U_ZERO_ERROR
#define U_ZERO_ERROR 0
#endif

#ifndef U_SUCCESS
#define U_SUCCESS(x) ((x) <= 0)
#endif

#ifndef U_FAILURE
#define U_FAILURE(x) ((x) > 0)
#endif

#ifndef ULOC_US
#define ULOC_US "en_US"
#endif

#ifndef UCOL_LESS
#define UCOL_LESS (-1)
#define UCOL_EQUAL 0
#define UCOL_GREATER 1
#endif

inline UCollator* ucol_open(const char*, UErrorCode* err) {
    if (err) *err = U_ZERO_ERROR;
    return reinterpret_cast<UCollator*>(1);
}

inline void ucol_close(UCollator*) {}

inline int ucol_strcoll(UCollator*, const UChar* s1, int32_t /*len1*/, const UChar* s2, int32_t /*len2*/) {
    if (!s1 && !s2) return UCOL_EQUAL;
    if (!s1) return UCOL_LESS;
    if (!s2) return UCOL_GREATER;
    int cmp = wcscmp(reinterpret_cast<const wchar_t*>(s1), reinterpret_cast<const wchar_t*>(s2));
    if (cmp < 0) return UCOL_LESS;
    if (cmp > 0) return UCOL_GREATER;
    return UCOL_EQUAL;
}
