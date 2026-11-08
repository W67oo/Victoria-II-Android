#pragma once

// =============================================================================
// cjk_word_break.hpp
// Victoria 2 Android Mobile CJK Smart Word Breaker & Kinsoku Shori (避头尾) Engine.
// Solves CJK text wrapping where ideographs have no delimiter spaces,
// while strictly enforcing typographic rules (forbidding line-start punctuation
// and line-end opening brackets) and preserving Latin words and digits unbroken.
// =============================================================================

#include <cstdint>
#include <cstddef>
#include <string_view>

namespace text {

// Checks whether a Unicode codepoint belongs to CJK ideographs, Kana, or Hangul.
inline constexpr bool is_cjk_codepoint(uint32_t cp) noexcept {
    return (cp >= 0x4E00 && cp <= 0x9FFF) ||   // CJK Unified Ideographs (Common Chinese characters)
           (cp >= 0x3400 && cp <= 0x4DBF) ||   // CJK Unified Ideographs Extension A
           (cp >= 0x20000 && cp <= 0x2A6DF) || // CJK Extension B
           (cp >= 0x3040 && cp <= 0x309F) ||   // Hiragana
           (cp >= 0x30A0 && cp <= 0x30FF) ||   // Katakana
           (cp >= 0x3100 && cp <= 0x312F) ||   // Bopomofo (Phonetic symbols)
           (cp >= 0xAC00 && cp <= 0xD7AF) ||   // Hangul Syllables
           (cp >= 0xF900 && cp <= 0xFAFF);     // CJK Compatibility Ideographs
}

// Characters forbidden at the BEGINNING of a line (Kinsoku Shori - 避头符)
// Line cannot start with sentence-closing punctuation, commas, or closing brackets.
inline constexpr bool is_kinsoku_bol(uint32_t cp) noexcept {
    switch (cp) {
        // ASCII punctuation
        case ',': case '.': case '!': case '?': case ';': case ':':
        case ')': case ']': case '}': case '>': case '%':
        // CJK fullwidth punctuation & brackets
        case 0x3001: // 、 (Ideographic comma)
        case 0x3002: // 。 (Ideographic full stop)
        case 0xFF0C: // ， (Fullwidth comma)
        case 0xFF0E: // ． (Fullwidth full stop)
        case 0xFF01: // ！ (Fullwidth exclamation mark)
        case 0xFF1F: // ？ (Fullwidth question mark)
        case 0xFF1B: // ； (Fullwidth semicolon)
        case 0xFF1A: // ： (Fullwidth colon)
        case 0xFF09: // ） (Fullwidth right parenthesis)
        case 0xFF3D: // ］ (Fullwidth right square bracket)
        case 0xFF5D: // ｝ (Fullwidth right curly bracket)
        case 0x3009: // 〉 (Right angle bracket)
        case 0x300B: // 》 (Right double angle bracket)
        case 0x300D: // 」 (Right corner bracket)
        case 0x300F: // 』 (Right white corner bracket)
        case 0x3011: // 】 (Right black lenticular bracket)
        case 0x3015: // 〕 (Right tortoise shell bracket)
        // Quotes, dashes and ellipses
        case 0x2019: // ’ (Right single quote)
        case 0x201D: // ” (Right double quote)
        case 0x2026: // … (Horizontal ellipsis)
        case 0x2014: // — (Em dash)
        case 0x00B0: // ° (Degree symbol)
        // Japanese small kana
        case 0x3041: case 0x3043: case 0x3045: case 0x3047: case 0x3049: // ぁぃぅぇぉ
        case 0x3063: case 0x3083: case 0x3085: case 0x3087: case 0x308E: // っゃゅょゎ
        case 0x30A1: case 0x30A3: case 0x30A5: case 0x30A7: case 0x30A9: // ァィゥェォ
        case 0x30C3: case 0x30E3: case 0x30E5: case 0x30E7: case 0x30EE: // ッャュョヮ
            return true;
        default:
            return false;
    }
}

// Characters forbidden at the END of a line (Kinsoku Shori - 避尾符)
// Line cannot end with opening brackets, currency symbols, or leading quotes.
inline constexpr bool is_kinsoku_eol(uint32_t cp) noexcept {
    switch (cp) {
        // ASCII opening brackets
        case '(': case '[': case '{': case '<':
        // CJK opening brackets
        case 0xFF08: // （ (Fullwidth left parenthesis)
        case 0xFF3B: // ［ (Fullwidth left square bracket)
        case 0xFF5B: // ｛ (Fullwidth left curly bracket)
        case 0x3008: // 〈 (Left angle bracket)
        case 0x300A: // 《 (Left double angle bracket)
        case 0x300C: // 「 (Left corner bracket)
        case 0x300E: // 『 (Left white corner bracket)
        case 0x3010: // 【 (Left black lenticular bracket)
        case 0x3014: // 〔 (Left tortoise shell bracket)
        // Opening quotes
        case 0x2018: // ‘ (Left single quote)
        case 0x201C: // “ (Left double quote)
        // Currency prefixes
        case '$':    // $
        case 0x00A3: // £
        case 0x00A5: // ¥
        case 0x20AC: // €
            return true;
        default:
            return false;
    }
}

inline constexpr bool is_ascii_word_token(uint32_t cp) noexcept {
    return (cp >= '0' && cp <= '9') ||
           (cp >= 'A' && cp <= 'Z') ||
           (cp >= 'a' && cp <= 'z') ||
           cp == '_' || cp == '\'';
}

inline constexpr bool is_whitespace_char(uint32_t cp) noexcept {
    return cp == ' ' || cp == '\t' || cp == '\r' || cp == '\n' || cp == 0x3000;
}

// Determines if a legal typographic line break is permitted between two adjacent characters.
inline constexpr bool can_break_between(uint32_t prev_cp, uint32_t next_cp) noexcept {
    // Mandatory break after newline
    if (prev_cp == '\n') return true;

    // Never break if prev character forbids line-end
    if (is_kinsoku_eol(prev_cp)) return false;

    // Never break if next character forbids line-start
    if (is_kinsoku_bol(next_cp)) return false;

    // Allow break after regular spaces
    if (prev_cp == ' ' || prev_cp == '\t' || prev_cp == 0x3000) return true;

    // Keep numbers and Latin words atomic
    if (is_ascii_word_token(prev_cp) && is_ascii_word_token(next_cp)) return false;

    // Allow break before or after any CJK ideograph
    if (is_cjk_codepoint(prev_cp) || is_cjk_codepoint(next_cp)) return true;

    // Allow break after Western hyphen
    if (prev_cp == '-' && !is_ascii_word_token(next_cp)) return true;

    return false;
}

} // namespace text
