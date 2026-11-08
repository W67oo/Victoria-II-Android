// =============================================================================
// icu_ubrk_ubidi_android.cpp
// Compact UAX #14 (CJK + Kinsoku Shori + Latin) BreakIterator & UAX #9 BiDi
// implementation for Android arm64-v8a (eliminates 32 MB icudt dependency).
// =============================================================================

#include <unicode/ubrk.h>
#include <unicode/ubidi.h>
#include "cjk_word_break.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

struct UBreakIterator {
    UBreakIteratorType type = UBRK_LINE;
    std::vector<int32_t> boundaries; // monotonically increasing UTF-16 offsets [0 .. len]
    size_t cursor = 0;
};

struct UBiDi {
    struct Run {
        int32_t logical_start = 0;
        int32_t length = 0;
        UBiDiDirection dir = UBIDI_LTR;
    };
    UBiDiLevel para_level = 0;
    std::vector<Run> visual_runs;
};

namespace {

inline uint32_t decode_utf16_at(const UChar* text, int32_t len, int32_t idx, int32_t& step) noexcept {
    step = 1;
    uint16_t u0 = static_cast<uint16_t>(text[idx]);
    if (u0 >= 0xD800 && u0 <= 0xDBFF && idx + 1 < len) {
        uint16_t u1 = static_cast<uint16_t>(text[idx + 1]);
        if (u1 >= 0xDC00 && u1 <= 0xDFFF) {
            step = 2;
            return 0x10000u + ((static_cast<uint32_t>(u0 - 0xD800) << 10) | static_cast<uint32_t>(u1 - 0xDC00));
        }
    }
    return u0;
}

inline bool is_combining_mark(uint32_t cp) noexcept {
    return (cp >= 0x0300 && cp <= 0x036F) ||
           (cp >= 0x0591 && cp <= 0x05C7) ||
           (cp >= 0x064B && cp <= 0x065F) ||
           (cp >= 0xFE00 && cp <= 0xFE0F) ||
           (cp == 0x200D);
}

inline bool is_cjk_ideograph_or_kana(uint32_t cp) noexcept {
    return text::is_cjk_codepoint(cp);
}

// Kinsoku Shori: closing / sentence-ending CJK & ASCII punctuation forbidden at line start
inline bool is_no_break_before(uint32_t cp) noexcept {
    return text::is_kinsoku_bol(cp);
}

// Kinsoku Shori: opening brackets/quotes forbidden at line end
inline bool is_no_break_after(uint32_t cp) noexcept {
    return text::is_kinsoku_eol(cp);
}

inline bool is_whitespace(uint32_t cp) noexcept {
    return cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == 0x3000 ||
           (cp >= 0x2000 && cp <= 0x200A);
}

inline bool is_word_alnum(uint32_t cp) noexcept {
    return (cp >= '0' && cp <= '9') ||
           (cp >= 'A' && cp <= 'Z') ||
           (cp >= 'a' && cp <= 'z') ||
           cp == '_' || cp == '\'' ||
           (cp >= 0x00C0 && cp <= 0x02AF) || // Latin Extended
           (cp >= 0x0400 && cp <= 0x04FF);   // Cyrillic
}

inline bool is_strong_rtl(uint32_t cp) noexcept {
    return (cp >= 0x0590 && cp <= 0x08FF) || // Hebrew, Arabic, Syriac, Thaana
           (cp >= 0xFB1D && cp <= 0xFDFF) || // Hebrew/Arabic Presentation Forms-A
           (cp >= 0xFE70 && cp <= 0xFEFF);   // Arabic Presentation Forms-B
}

void build_boundaries(UBreakIterator& bi, const UChar* text, int32_t len) {
    bi.boundaries.clear();
    if (!text || len <= 0) {
        bi.boundaries.push_back(0);
        return;
    }

    // First decode grapheme clusters
    struct Cluster {
        int32_t start;
        int32_t end;
        uint32_t lead_cp;
    };
    std::vector<Cluster> clusters;
    clusters.reserve(static_cast<size_t>(len));

    int32_t i = 0;
    while (i < len) {
        int32_t step = 1;
        uint32_t cp = decode_utf16_at(text, len, i, step);
        int32_t c_start = i;
        i += step;
        while (i < len) {
            int32_t mstep = 1;
            uint32_t mcp = decode_utf16_at(text, len, i, mstep);
            if (!is_combining_mark(mcp)) break;
            i += mstep;
        }
        clusters.push_back({c_start, i, cp});
    }

    bi.boundaries.push_back(0);
    if (bi.type == UBRK_CHARACTER) {
        for (const auto& c : clusters) {
            bi.boundaries.push_back(c.end);
        }
        return;
    }

    if (bi.type == UBRK_WORD) {
        for (size_t k = 0; k < clusters.size(); ++k) {
            bool brk = true;
            if (k + 1 < clusters.size()) {
                uint32_t a = clusters[k].lead_cp;
                uint32_t b = clusters[k + 1].lead_cp;
                if (is_word_alnum(a) && is_word_alnum(b)) {
                    brk = false;
                } else if (is_whitespace(a) && is_whitespace(b)) {
                    brk = false;
                }
            }
            if (brk) {
                bi.boundaries.push_back(clusters[k].end);
            }
        }
        if (bi.boundaries.back() != len) bi.boundaries.push_back(len);
        return;
    }

    // UBRK_LINE / UBRK_SENTENCE: UAX #14 CJK + Kinsoku Shori + Latin whitespace/hyphen
    for (size_t k = 0; k < clusters.size(); ++k) {
        if (k + 1 == clusters.size()) {
            bi.boundaries.push_back(len);
            break;
        }
        uint32_t curr = clusters[k].lead_cp;
        uint32_t next = clusters[k + 1].lead_cp;

        // Mandatory break after newline
        if (curr == '\n') {
            bi.boundaries.push_back(clusters[k].end);
            continue;
        }
        if (curr == '\r' && next == '\n') {
            continue;
        }
        // Absorb trailing spaces onto the current word before breaking
        if (is_whitespace(next)) {
            continue;
        }
        if (is_whitespace(curr)) {
            bi.boundaries.push_back(clusters[k].end);
            continue;
        }
        // Kinsoku Shori prohibitions
        if (is_no_break_after(curr) || is_no_break_before(next) || curr == 0x00A0) {
            continue;
        }
        // Break after hyphen or CJK punctuation, or before/after any CJK ideograph/kana
        if (curr == '-' ||
            is_cjk_ideograph_or_kana(curr) ||
            is_cjk_ideograph_or_kana(next) ||
            is_no_break_before(curr)) {
            bi.boundaries.push_back(clusters[k].end);
        }
    }

    if (bi.boundaries.back() != len) {
        bi.boundaries.push_back(len);
    }
}

} // namespace

extern "C" {

UBreakIterator* ubrk_open(UBreakIteratorType type, const char* /*locale*/,
                          const UChar* text, int32_t textLength, UErrorCode* status) {
    if (status && U_FAILURE(*status)) return nullptr;
    auto* bi = new UBreakIterator();
    bi->type = type;
    if (text && textLength >= 0) {
        build_boundaries(*bi, text, textLength);
    } else {
        bi->boundaries.push_back(0);
    }
    if (status) *status = U_ZERO_ERROR;
    return bi;
}

int32_t ubrk_getBinaryRules(UBreakIterator* bi, uint8_t* binaryRules,
                            int32_t rulesCapacity, UErrorCode* status) {
    if (!bi) {
        if (status) *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    constexpr int32_t kRuleBytes = static_cast<int32_t>(sizeof(int32_t));
    if (binaryRules == nullptr && rulesCapacity == 0) {
        if (status) *status = U_ZERO_ERROR;
        return kRuleBytes;
    }
    if (rulesCapacity < kRuleBytes || binaryRules == nullptr) {
        if (status) *status = U_BUFFER_OVERFLOW_ERROR;
        return kRuleBytes;
    }
    int32_t tag = static_cast<int32_t>(bi->type);
    std::memcpy(binaryRules, &tag, sizeof(tag));
    if (status) *status = U_ZERO_ERROR;
    return kRuleBytes;
}

UBreakIterator* ubrk_openBinaryRules(const uint8_t* binaryRules, int32_t rulesLength,
                                     const UChar* text, int32_t textLength, UErrorCode* status) {
    if (status && U_FAILURE(*status)) return nullptr;
    if (!binaryRules || rulesLength < static_cast<int32_t>(sizeof(int32_t))) {
        if (status) *status = U_ILLEGAL_ARGUMENT_ERROR;
        return nullptr;
    }
    int32_t tag = 0;
    std::memcpy(&tag, binaryRules, sizeof(tag));
    auto* bi = new UBreakIterator();
    bi->type = static_cast<UBreakIteratorType>(tag);
    int32_t actual_len = textLength;
    if (text && actual_len < 0) {
        actual_len = 0;
        while (text[actual_len] != 0) ++actual_len;
    }
    build_boundaries(*bi, text, std::max<int32_t>(0, actual_len));
    if (status) *status = U_ZERO_ERROR;
    return bi;
}

void ubrk_close(UBreakIterator* bi) {
    delete bi;
}

int32_t ubrk_first(UBreakIterator* bi) {
    if (!bi || bi->boundaries.empty()) return 0;
    bi->cursor = 0;
    return bi->boundaries[0];
}

int32_t ubrk_next(UBreakIterator* bi) {
    if (!bi || bi->boundaries.empty()) return UBRK_DONE;
    if (bi->cursor + 1 >= bi->boundaries.size()) {
        bi->cursor = bi->boundaries.size();
        return UBRK_DONE;
    }
    ++bi->cursor;
    return bi->boundaries[bi->cursor];
}

int32_t ubrk_previous(UBreakIterator* bi) {
    if (!bi || bi->boundaries.empty() || bi->cursor == 0) return UBRK_DONE;
    --bi->cursor;
    return bi->boundaries[bi->cursor];
}

UBiDi* ubidi_open() {
    return new UBiDi();
}

void ubidi_close(UBiDi* pBiDi) {
    delete pBiDi;
}

void ubidi_setPara(UBiDi* pBiDi, const UChar* text, int32_t length,
                   UBiDiLevel paraLevel, UBiDiLevel* /*embeddingLevels*/, UErrorCode* pErrorCode) {
    if (!pBiDi) {
        if (pErrorCode) *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    pBiDi->para_level = paraLevel;
    pBiDi->visual_runs.clear();

    if (!text || length <= 0) {
        if (pErrorCode) *pErrorCode = U_ZERO_ERROR;
        return;
    }

    UBiDiDirection default_dir = (paraLevel & 1) ? UBIDI_RTL : UBIDI_LTR;
    int32_t idx = 0;
    while (idx < length) {
        int32_t step = 1;
        uint32_t cp = decode_utf16_at(text, length, idx, step);
        UBiDiDirection dir = is_strong_rtl(cp) ? UBIDI_RTL : default_dir;
        if (!pBiDi->visual_runs.empty() && pBiDi->visual_runs.back().dir == dir) {
            pBiDi->visual_runs.back().length += step;
        } else {
            pBiDi->visual_runs.push_back({idx, step, dir});
        }
        idx += step;
    }

    if (default_dir == UBIDI_RTL && pBiDi->visual_runs.size() > 1) {
        std::reverse(pBiDi->visual_runs.begin(), pBiDi->visual_runs.end());
    }
    if (pErrorCode) *pErrorCode = U_ZERO_ERROR;
}

int32_t ubidi_countRuns(UBiDi* pBiDi, UErrorCode* pErrorCode) {
    if (!pBiDi) {
        if (pErrorCode) *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if (pErrorCode) *pErrorCode = U_ZERO_ERROR;
    return static_cast<int32_t>(pBiDi->visual_runs.size());
}

UBiDiDirection ubidi_getVisualRun(UBiDi* pBiDi, int32_t runIndex,
                                  int32_t* pLogicalStart, int32_t* pLength) {
    if (!pBiDi || runIndex < 0 || static_cast<size_t>(runIndex) >= pBiDi->visual_runs.size()) {
        if (pLogicalStart) *pLogicalStart = 0;
        if (pLength) *pLength = 0;
        return UBIDI_LTR;
    }
    const auto& r = pBiDi->visual_runs[static_cast<size_t>(runIndex)];
    if (pLogicalStart) *pLogicalStart = r.logical_start;
    if (pLength) *pLength = r.length;
    return r.dir;
}

} // extern "C"
