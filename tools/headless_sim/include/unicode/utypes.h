#pragma once
// =============================================================================
// tools/headless_sim/include/unicode/utypes.h
// Self-contained ICU core type declarations for headless simulation host build
// =============================================================================

#include <cstdint>
#include <cwchar>

#ifndef UErrorCode
typedef int32_t UErrorCode;
#endif

#ifndef U_ZERO_ERROR
#define U_ZERO_ERROR 0
#endif

#ifndef U_ILLEGAL_ARGUMENT_ERROR
#define U_ILLEGAL_ARGUMENT_ERROR 1
#endif

#ifndef U_BUFFER_OVERFLOW_ERROR
#define U_BUFFER_OVERFLOW_ERROR 15
#endif

#ifndef U_SUCCESS
#define U_SUCCESS(x) ((x) <= 0)
#endif

#ifndef U_FAILURE
#define U_FAILURE(x) ((x) > 0)
#endif

#ifndef UChar
typedef char16_t UChar;
#endif

#ifndef UBool
typedef int8_t UBool;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif
