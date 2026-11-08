#pragma once
// =============================================================================
// arm64_shim/unicode/utypes.h
// Lightweight ICU C API type declarations for Android arm64-v8a (zero icudt bloat)
// =============================================================================

#include <cstdint>

using UChar = char16_t;
using UChar32 = int32_t;
using UBool = int8_t;

enum UErrorCode : int32_t {
    U_ZERO_ERROR = 0,
    U_ILLEGAL_ARGUMENT_ERROR = 1,
    U_MEMORY_ALLOCATION_ERROR = 7,
    U_BUFFER_OVERFLOW_ERROR = 15
};

inline constexpr bool U_SUCCESS(UErrorCode code) noexcept { return code <= U_ZERO_ERROR; }
inline constexpr bool U_FAILURE(UErrorCode code) noexcept { return code > U_ZERO_ERROR; }
