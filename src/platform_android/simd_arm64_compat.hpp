#pragma once
// =============================================================================
// simd_arm64_compat.hpp
// Force-included on ARM64 (__aarch64__) builds so Project Alice & DataContainer
// x86 SSE/SSE3/SSE4.1 intrinsics in ve.hpp / ve_sse3.hpp compile directly into
// 128-bit native ARMv8 NEON instructions via SIMDe, and provides C++20
// std::atomic_ref<T> when building against NDK r27 (LLVM 18 libc++).
// =============================================================================

#if defined(__aarch64__) || defined(_M_ARM64)

#ifndef SIMDE_ENABLE_NATIVE_ALIASES
#define SIMDE_ENABLE_NATIVE_ALIASES 1
#endif

// Prevent Random123 from attempting raw x86 <xmmintrin.h> / <emmintrin.h>
#define R123_USE_XMMINTRIN_H 0
#define R123_USE_EMMINTRIN_H 0
#define R123_USE_SSE 0

#if __has_include(<simde/x86/sse4.1.h>)
#include <simde/x86/sse.h>
#include <simde/x86/sse2.h>
#include <simde/x86/sse3.h>
#include <simde/x86/ssse3.h>
#include <simde/x86/sse4.1.h>
#endif

#ifdef __cplusplus
#include <atomic>
#include <bit>
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <type_traits>

// NDK r27 (LLVM 18 libc++) does not yet define C++20 std::atomic_ref<T>.
// Provide a lock-free Clang __atomic_* implementation for integral & float types.
#if defined(_LIBCPP_VERSION) && !defined(__cpp_lib_atomic_ref)
#ifdef _LIBCPP_BEGIN_NAMESPACE_STD
_LIBCPP_BEGIN_NAMESPACE_STD
#else
namespace std {
#endif

namespace vic2_detail {
inline constexpr int to_builtin_order(memory_order m) noexcept {
    switch (m) {
        case memory_order_relaxed: return __ATOMIC_RELAXED;
        case memory_order_consume: return __ATOMIC_CONSUME;
        case memory_order_acquire: return __ATOMIC_ACQUIRE;
        case memory_order_release: return __ATOMIC_RELEASE;
        case memory_order_acq_rel: return __ATOMIC_ACQ_REL;
        case memory_order_seq_cst: return __ATOMIC_SEQ_CST;
    }
    return __ATOMIC_SEQ_CST;
}
} // namespace vic2_detail

template <typename T>
class atomic_ref {
    static_assert(is_trivially_copyable_v<T>, "atomic_ref requires trivially copyable T");
    T* ptr_;

public:
    using value_type = T;
    static constexpr size_t required_alignment = alignof(T);
    static constexpr bool is_always_lock_free = (sizeof(T) <= 8);

    explicit atomic_ref(T& obj) noexcept : ptr_(&obj) {}
    atomic_ref(const atomic_ref&) noexcept = default;
    atomic_ref& operator=(const atomic_ref&) = delete;

    bool is_lock_free() const noexcept { return __atomic_is_lock_free(sizeof(T), ptr_); }

    void store(T desired, memory_order order = memory_order_seq_cst) const noexcept {
        if constexpr (is_integral_v<T> || is_pointer_v<T>) {
            __atomic_store_n(ptr_, desired, vic2_detail::to_builtin_order(order));
        } else {
            __atomic_store(ptr_, &desired, vic2_detail::to_builtin_order(order));
        }
    }

    T load(memory_order order = memory_order_seq_cst) const noexcept {
        if constexpr (is_integral_v<T> || is_pointer_v<T>) {
            return __atomic_load_n(ptr_, vic2_detail::to_builtin_order(order));
        } else {
            T ret;
            __atomic_load(ptr_, &ret, vic2_detail::to_builtin_order(order));
            return ret;
        }
    }

    operator T() const noexcept { return load(); }
    T operator=(T desired) const noexcept { store(desired); return desired; }

    T exchange(T desired, memory_order order = memory_order_seq_cst) const noexcept {
        if constexpr (is_integral_v<T> || is_pointer_v<T>) {
            return __atomic_exchange_n(ptr_, desired, vic2_detail::to_builtin_order(order));
        } else {
            T ret;
            __atomic_exchange(ptr_, &desired, &ret, vic2_detail::to_builtin_order(order));
            return ret;
        }
    }

    bool compare_exchange_weak(T& expected, T desired,
                               memory_order success = memory_order_seq_cst,
                               memory_order failure = memory_order_seq_cst) const noexcept {
        return __atomic_compare_exchange(ptr_, &expected, &desired, true,
                                         vic2_detail::to_builtin_order(success),
                                         vic2_detail::to_builtin_order(failure));
    }

    bool compare_exchange_strong(T& expected, T desired,
                                 memory_order success = memory_order_seq_cst,
                                 memory_order failure = memory_order_seq_cst) const noexcept {
        return __atomic_compare_exchange(ptr_, &expected, &desired, false,
                                         vic2_detail::to_builtin_order(success),
                                         vic2_detail::to_builtin_order(failure));
    }

    T fetch_add(T arg, memory_order order = memory_order_seq_cst) const noexcept {
        if constexpr (is_integral_v<T>) {
            return __atomic_fetch_add(ptr_, arg, vic2_detail::to_builtin_order(order));
        } else if constexpr (is_floating_point_v<T>) {
            using UInt = conditional_t<sizeof(T) == 4, uint32_t, uint64_t>;
            UInt* uptr = reinterpret_cast<UInt*>(ptr_);
            UInt old_bits = __atomic_load_n(uptr, __ATOMIC_RELAXED);
            while (true) {
                T old_val = bit_cast<T>(old_bits);
                T new_val = old_val + arg;
                UInt new_bits = bit_cast<UInt>(new_val);
                if (__atomic_compare_exchange_n(uptr, &old_bits, new_bits, true,
                                                vic2_detail::to_builtin_order(order),
                                                __ATOMIC_RELAXED)) {
                    return old_val;
                }
            }
        }
    }

    T fetch_sub(T arg, memory_order order = memory_order_seq_cst) const noexcept {
        if constexpr (is_integral_v<T>) {
            return __atomic_fetch_sub(ptr_, arg, vic2_detail::to_builtin_order(order));
        } else if constexpr (is_floating_point_v<T>) {
            return fetch_add(-arg, order);
        }
    }

    T fetch_and(T arg, memory_order order = memory_order_seq_cst) const noexcept {
        static_assert(is_integral_v<T>);
        return __atomic_fetch_and(ptr_, arg, vic2_detail::to_builtin_order(order));
    }

    T fetch_or(T arg, memory_order order = memory_order_seq_cst) const noexcept {
        static_assert(is_integral_v<T>);
        return __atomic_fetch_or(ptr_, arg, vic2_detail::to_builtin_order(order));
    }

    T fetch_xor(T arg, memory_order order = memory_order_seq_cst) const noexcept {
        static_assert(is_integral_v<T>);
        return __atomic_fetch_xor(ptr_, arg, vic2_detail::to_builtin_order(order));
    }

    T operator+=(T arg) const noexcept { return fetch_add(arg) + arg; }
    T operator-=(T arg) const noexcept { return fetch_sub(arg) - arg; }
    T operator&=(T arg) const noexcept { return fetch_and(arg) & arg; }
    T operator|=(T arg) const noexcept { return fetch_or(arg) | arg; }
    T operator^=(T arg) const noexcept { return fetch_xor(arg) ^ arg; }
};

template <typename T>
atomic_ref(T&) -> atomic_ref<T>;

#ifdef _LIBCPP_END_NAMESPACE_STD
_LIBCPP_END_NAMESPACE_STD
#else
} // namespace std
#endif
#define __cpp_lib_atomic_ref 201806L
#endif // !_cpp_lib_atomic_ref

// NDK r27 (LLVM 18 libc++) lacks floating-point std::from_chars overloads.
// Provide bounded, slice-safe C++17 floating-point std::from_chars in libc++'s inline namespace.
#if defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 200000 && !defined(_LIBCPP___CHARCONV_FROM_CHARS_FLOATING_POINT_H)
#ifdef _LIBCPP_BEGIN_NAMESPACE_STD
_LIBCPP_BEGIN_NAMESPACE_STD
#else
namespace std {
#endif

namespace vic2_charconv_detail {

inline bool is_ascii_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

inline bool is_ascii_hex_digit(char c) noexcept {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

template <typename FloatT>
inline from_chars_result parse_floating_from_chars(const char* first, const char* last,
                                                   FloatT& value, chars_format fmt) noexcept {
    if (first >= last) {
        return {first, errc::invalid_argument};
    }

    const char* p = first;
    if (*p == '-') {
        ++p;
        if (p == last) {
            return {first, errc::invalid_argument};
        }
    }

    const bool is_hex = (fmt == chars_format::hex);
    const bool allow_exp = is_hex ||
        ((static_cast<unsigned>(fmt) & static_cast<unsigned>(chars_format::scientific)) != 0u);
    const bool require_exp = (fmt == chars_format::scientific);

    bool has_digits = false;
    if (is_hex) {
        while (p < last && is_ascii_hex_digit(*p)) {
            has_digits = true;
            ++p;
        }
        if (p < last && *p == '.') {
            ++p;
            while (p < last && is_ascii_hex_digit(*p)) {
                has_digits = true;
                ++p;
            }
        }
    } else {
        while (p < last && is_ascii_digit(*p)) {
            has_digits = true;
            ++p;
        }
        if (p < last && *p == '.') {
            ++p;
            while (p < last && is_ascii_digit(*p)) {
                has_digits = true;
                ++p;
            }
        }
    }

    if (!has_digits) {
        return {first, errc::invalid_argument};
    }

    bool has_exp = false;
    if (allow_exp && p < last) {
        const char exp_ch = *p;
        const bool match_exp = is_hex ? (exp_ch == 'p' || exp_ch == 'P')
                                      : (exp_ch == 'e' || exp_ch == 'E');
        if (match_exp) {
            const char* exp_start = p;
            ++p;
            if (p < last && (*p == '+' || *p == '-')) {
                ++p;
            }
            const char* exp_digits = p;
            while (p < last && is_ascii_digit(*p)) {
                ++p;
            }
            if (p > exp_digits) {
                has_exp = true;
            } else {
                p = exp_start;
            }
        }
    }

    if (require_exp && !has_exp) {
        return {first, errc::invalid_argument};
    }

    const size_t token_len = static_cast<size_t>(p - first);
    char stack_buf[256];
    char* buf = stack_buf;
    const bool use_heap = is_hex ? (token_len + 3 >= sizeof(stack_buf))
                                 : (token_len + 1 >= sizeof(stack_buf));
    if (use_heap) {
        buf = static_cast<char*>(::malloc(token_len + 4));
        if (!buf) {
            return {first, errc::not_enough_memory};
        }
    }

    size_t out_idx = 0;
    size_t in_idx = 0;
    if (first[0] == '-') {
        buf[out_idx++] = '-';
        in_idx = 1;
    }
    if (is_hex) {
        buf[out_idx++] = '0';
        buf[out_idx++] = 'x';
    }
    const size_t rem = token_len - in_idx;
    ::memcpy(buf + out_idx, first + in_idx, rem);
    buf[out_idx + rem] = '\0';

    errno = 0;
    char* endptr = nullptr;
    FloatT parsed{};
    if constexpr (is_same_v<FloatT, float>) {
        parsed = ::strtof(buf, &endptr);
    } else if constexpr (is_same_v<FloatT, double>) {
        parsed = ::strtod(buf, &endptr);
    } else {
        parsed = ::strtold(buf, &endptr);
    }
    const int saved_errno = errno;
    const bool parse_ok = (endptr != buf);

    if (use_heap) {
        ::free(buf);
    }

    if (!parse_ok) {
        return {first, errc::invalid_argument};
    }
    if (saved_errno == ERANGE) {
        return {p, errc::result_out_of_range};
    }
    value = parsed;
    return {p, errc{}};
}

} // namespace vic2_charconv_detail

inline from_chars_result from_chars(const char* first, const char* last, float& value,
                                    chars_format fmt = chars_format::general) noexcept {
    return vic2_charconv_detail::parse_floating_from_chars(first, last, value, fmt);
}

inline from_chars_result from_chars(const char* first, const char* last, double& value,
                                    chars_format fmt = chars_format::general) noexcept {
    return vic2_charconv_detail::parse_floating_from_chars(first, last, value, fmt);
}

inline from_chars_result from_chars(const char* first, const char* last, long double& value,
                                    chars_format fmt = chars_format::general) noexcept {
    return vic2_charconv_detail::parse_floating_from_chars(first, last, value, fmt);
}

#ifdef _LIBCPP_END_NAMESPACE_STD
_LIBCPP_END_NAMESPACE_STD
#else
} // namespace std
#endif
#endif // libc++ < 20 floating-point std::from_chars
#endif // __cplusplus

#endif // __aarch64__
