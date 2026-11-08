// =============================================================================
// gbk_transcoder.hpp
// Fast Heuristic Charset Detector and Zero-Dependency GBK/CP936 -> UTF-8
// Transcoder for Victoria 2 Mobile Legacy Chinese Localization Support.
// =============================================================================
#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace text {

enum class CharsetEncoding : uint8_t {
    Ascii,
    Utf8,
    Utf8Bom,
    Gbk,
    Windows1252
};

// Heuristically detect encoding of raw byte buffer (e.g. CSV file content)
CharsetEncoding detect_charset_encoding(const char* data, size_t size) noexcept;

// Convert single GBK/CP936 double-byte to Unicode (UCS-2) codepoint
uint16_t gbk_pair_to_unicode(uint8_t lead, uint8_t trail) noexcept;

// Convert GBK/CP936 byte stream to UTF-8
std::string gbk_to_utf8(std::string_view gbk_text);

// Check if raw byte buffer conforms to strictly valid UTF-8
bool is_valid_utf8(const char* data, size_t size, bool& has_multibyte) noexcept;

// Convert raw buffer to UTF-8 based on automatic detection:
// - If already UTF-8 / ASCII: returns string view as-is (or strips BOM)
// - If GBK / CP936: transcodes to UTF-8
// - If Windows-1252: transcodes to UTF-8
std::string ensure_utf8_string(std::string_view input, CharsetEncoding forced_hint = CharsetEncoding::Ascii);

} // namespace text
