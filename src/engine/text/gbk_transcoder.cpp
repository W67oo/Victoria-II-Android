// =============================================================================
// gbk_transcoder.cpp
// Fast Heuristic Charset Detector and Zero-Dependency GBK/CP936 -> UTF-8
// Transcoder for Victoria 2 Mobile Legacy Chinese Localization Support.
// =============================================================================
#include "gbk_transcoder.hpp"
#include "gbk_table.hpp"
#include "text.hpp"

#include <algorithm>
#include <vector>

namespace text {

uint16_t gbk_pair_to_unicode(uint8_t lead, uint8_t trail) noexcept {
    if (lead < 0x81 || lead > 0xFE) return 0;
    if (trail < 0x40 || trail == 0x7F || trail > 0xFE) return 0;
    uint32_t trail_offset = (trail < 0x7F) ? uint32_t(trail - 0x40) : uint32_t(trail - 0x41);
    uint32_t index = (uint32_t(lead - 0x81) * 190u) + trail_offset;
    if (index >= 23940) return 0;
    return gbk_data::gbk_to_unicode_table[index];
}

bool is_valid_utf8(const char* data, size_t size, bool& has_multibyte) noexcept {
    has_multibyte = false;
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);
    size_t i = 0;

    while (i < size) {
        uint8_t b1 = ptr[i];
        if (b1 < 0x80) {
            ++i;
            continue;
        }

        has_multibyte = true;

        if ((b1 & 0xE0) == 0xC0) {
            // 2-byte sequence: 110xxxxx 10xxxxxx
            if (b1 < 0xC2) return false; // Overlong encoding
            if (i + 1 >= size) return false;
            uint8_t b2 = ptr[i + 1];
            if ((b2 & 0xC0) != 0x80) return false;
            i += 2;
        } else if ((b1 & 0xF0) == 0xE0) {
            // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
            if (i + 2 >= size) return false;
            uint8_t b2 = ptr[i + 1];
            uint8_t b3 = ptr[i + 2];
            if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return false;
            if (b1 == 0xE0 && b2 < 0xA0) return false; // Overlong
            if (b1 == 0xED && b2 >= 0xA0) return false; // UTF-16 surrogates U+D800..U+DFFF
            i += 3;
        } else if ((b1 & 0xF8) == 0xF0) {
            // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            if (i + 3 >= size) return false;
            uint8_t b2 = ptr[i + 1];
            uint8_t b3 = ptr[i + 2];
            uint8_t b4 = ptr[i + 3];
            if ((b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80 || (b4 & 0xC0) != 0x80) return false;
            if (b1 == 0xF0 && b2 < 0x90) return false; // Overlong
            if (b1 == 0xF4 && b2 > 0x8F) return false; // Out of Unicode range (> U+10FFFF)
            if (b1 > 0xF4) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

CharsetEncoding detect_charset_encoding(const char* data, size_t size) noexcept {
    if (!data || size == 0) return CharsetEncoding::Ascii;

    // 1. Check UTF-8 BOM
    if (size >= 3 &&
        uint8_t(data[0]) == 0xEF &&
        uint8_t(data[1]) == 0xBB &&
        uint8_t(data[2]) == 0xBF) {
        return CharsetEncoding::Utf8Bom;
    }

    // 2. Test strictly valid UTF-8
    bool has_mb = false;
    if (is_valid_utf8(data, size, has_mb)) {
        return has_mb ? CharsetEncoding::Utf8 : CharsetEncoding::Ascii;
    }

    // 3. Test for GBK/CP936 sequences
    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);
    size_t gbk_valid_pairs = 0;
    size_t high_bytes_total = 0;

    for (size_t i = 0; i < size; ) {
        uint8_t b1 = ptr[i];
        if (b1 < 0x80) {
            ++i;
            continue;
        }

        ++high_bytes_total;

        // Skip color escape code in Victoria 2: 0xA7 followed by color char
        if (b1 == 0xA7 && (i + 1 < size)) {
            char next = static_cast<char>(ptr[i + 1]);
            if (next == 'R' || next == 'G' || next == 'Y' || next == 'W' || next == 'B' ||
                next == 'O' || next == '!' || (next >= '0' && next <= '9') ||
                next == 'C' || next == 'T' || next == 'b' || next == 'g' || next == 'r') {
                i += 2;
                continue;
            }
        }

        if (b1 >= 0x81 && b1 <= 0xFE && (i + 1 < size)) {
            uint8_t b2 = ptr[i + 1];
            if ((b2 >= 0x40 && b2 <= 0x7E) || (b2 >= 0x80 && b2 <= 0xFE)) {
                ++gbk_valid_pairs;
                ++high_bytes_total; // Count second byte
                i += 2;
                continue;
            }
        }
        ++i;
    }

    // If we observed valid GBK pairs and they account for the majority of high bytes
    if (gbk_valid_pairs > 0 && (gbk_valid_pairs * 2 >= high_bytes_total / 2)) {
        return CharsetEncoding::Gbk;
    }

    return CharsetEncoding::Windows1252;
}

std::string gbk_to_utf8(std::string_view gbk_text) {
    std::string result;
    result.reserve(gbk_text.size() * 3 / 2);

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(gbk_text.data());
    const size_t len = gbk_text.size();

    for (size_t i = 0; i < len; ) {
        uint8_t c = ptr[i];

        // 1. Preserve Victoria 2 color escape: 0xA7 (§) followed by color char
        if (c == 0xA7 && (i + 1 < len)) {
            char next = static_cast<char>(ptr[i + 1]);
            if (next == 'R' || next == 'G' || next == 'Y' || next == 'W' || next == 'B' ||
                next == 'O' || next == '!' || (next >= '0' && next <= '9') ||
                next == 'C' || next == 'T' || next == 'b' || next == 'g' || next == 'r') {
                result.push_back(static_cast<char>(0xC2));
                result.push_back(static_cast<char>(0xA7));
                result.push_back(next);
                i += 2;
                continue;
            }
        }

        // 2. ASCII characters pass through directly
        if (c < 0x80) {
            result.push_back(static_cast<char>(c));
            ++i;
            continue;
        }

        // 3. GBK double-byte sequence
        if (c >= 0x81 && c <= 0xFE && (i + 1 < len)) {
            uint8_t trail = ptr[i + 1];
            uint16_t ucs = gbk_pair_to_unicode(c, trail);
            if (ucs != 0) {
                if (ucs <= 0x007F) {
                    result.push_back(static_cast<char>(ucs));
                } else if (ucs <= 0x07FF) {
                    result.push_back(static_cast<char>(0xC0 | (ucs >> 6)));
                    result.push_back(static_cast<char>(0x80 | (ucs & 0x3F)));
                } else {
                    result.push_back(static_cast<char>(0xE0 | (ucs >> 12)));
                    result.push_back(static_cast<char>(0x80 | ((ucs >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (ucs & 0x3F)));
                }
                i += 2;
                continue;
            }
        }

        // 4. Lone high byte: fallback map via Windows-1250/1252 to UTF-8
        auto ucs = static_cast<uint16_t>(text::win1250toUTF16(static_cast<char>(c)));
        if (ucs <= 0x007F) {
            result.push_back(static_cast<char>(ucs));
        } else if (ucs <= 0x07FF) {
            result.push_back(static_cast<char>(0xC0 | (ucs >> 6)));
            result.push_back(static_cast<char>(0x80 | (ucs & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xE0 | (ucs >> 12)));
            result.push_back(static_cast<char>(0x80 | ((ucs >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (ucs & 0x3F)));
        }
        ++i;
    }

    return result;
}

std::string ensure_utf8_string(std::string_view input, CharsetEncoding forced_hint) {
    if (input.empty()) return std::string();

    CharsetEncoding enc = (forced_hint != CharsetEncoding::Ascii)
        ? forced_hint
        : detect_charset_encoding(input.data(), input.size());

    switch (enc) {
    case CharsetEncoding::Utf8Bom:
        return std::string(input.substr(3));

    case CharsetEncoding::Ascii:
    case CharsetEncoding::Utf8:
        return std::string(input);

    case CharsetEncoding::Gbk:
        return gbk_to_utf8(input);

    case CharsetEncoding::Windows1252: {
        std::string res;
        res.reserve(input.size() * 3 / 2);
        for (char ch : input) {
            auto ucs = static_cast<uint16_t>(text::win1250toUTF16(ch));
            if (ucs <= 0x007F) {
                res.push_back(static_cast<char>(ucs));
            } else if (ucs <= 0x07FF) {
                res.push_back(static_cast<char>(0xC0 | (ucs >> 6)));
                res.push_back(static_cast<char>(0x80 | (ucs & 0x3F)));
            } else {
                res.push_back(static_cast<char>(0xE0 | (ucs >> 12)));
                res.push_back(static_cast<char>(0x80 | ((ucs >> 6) & 0x3F)));
                res.push_back(static_cast<char>(0x80 | (ucs & 0x3F)));
            }
        }
        return res;
    }
    }
    return std::string(input);
}

} // namespace text
