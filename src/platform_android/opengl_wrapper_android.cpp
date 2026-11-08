// =============================================================================
// opengl_wrapper_android.cpp
// OpenGL ES 3.2 Context Management, GLSL 460 -> GLSL ES 3.20 Transpiler, and
// Transparent S3TC (DXT1/DXT3/DXT5) -> RGBA8 2D & 2D-Array Software Decoder
// =============================================================================

#define VIC2_GLES_WRAPPER_IMPL 1
#include "glew.h"
#include "runtime_config_android.hpp"
#include "opengl_wrapper.hpp"
#include "system_state.hpp"

#include <SDL.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace vic2_mobile::gles {

inline bool supports_hardware_s3tc() noexcept {
    static int cached = -1;
    if (cached < 0) {
        const char* ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        cached = (ext && (std::strstr(ext, "GL_EXT_texture_compression_s3tc") != nullptr ||
                          std::strstr(ext, "GL_WEBGL_compressed_texture_s3tc") != nullptr))
                     ? 1
                     : 0;
        VIC2_LOGI("GPU='%s', GL='%s', Hardware S3TC=%d",
                  glGetString(GL_RENDERER) ? reinterpret_cast<const char*>(glGetString(GL_RENDERER)) : "unknown",
                  glGetString(GL_VERSION)  ? reinterpret_cast<const char*>(glGetString(GL_VERSION))  : "unknown",
                  cached);
    }
    return cached == 1;
}

// Fast 4x4 RGB565 block unpacker for DXT1 / DXT3 / DXT5 -> RGBA8888
inline void decode_dxt_color_block(const uint8_t* block, uint32_t out_rgba[16], bool dxt1_alpha_mode) noexcept {
    uint16_t c0 = static_cast<uint16_t>(block[0] | (block[1] << 8));
    uint16_t c1 = static_cast<uint16_t>(block[2] | (block[3] << 8));

    auto unpack565 = [](uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
        r = static_cast<uint8_t>(((c >> 11) & 0x1F) * 255 / 31);
        g = static_cast<uint8_t>(((c >> 5)  & 0x3F) * 255 / 63);
        b = static_cast<uint8_t>(( c        & 0x1F) * 255 / 31);
    };

    uint8_t r[4], g[4], b[4], a[4] = {255, 255, 255, 255};
    unpack565(c0, r[0], g[0], b[0]);
    unpack565(c1, r[1], g[1], b[1]);

    if (c0 > c1 || !dxt1_alpha_mode) {
        r[2] = static_cast<uint8_t>((2 * r[0] + r[1]) / 3);
        g[2] = static_cast<uint8_t>((2 * g[0] + g[1]) / 3);
        b[2] = static_cast<uint8_t>((2 * b[0] + b[1]) / 3);
        r[3] = static_cast<uint8_t>((r[0] + 2 * r[1]) / 3);
        g[3] = static_cast<uint8_t>((g[0] + 2 * g[1]) / 3);
        b[3] = static_cast<uint8_t>((b[0] + 2 * b[1]) / 3);
    } else {
        r[2] = static_cast<uint8_t>((r[0] + r[1]) / 2);
        g[2] = static_cast<uint8_t>((g[0] + g[1]) / 2);
        b[2] = static_cast<uint8_t>((b[0] + b[1]) / 2);
        r[3] = 0; g[3] = 0; b[3] = 0; a[3] = 0;
    }

    uint32_t indices = static_cast<uint32_t>(
        block[4] | (block[5] << 8) | (block[6] << 16) | (block[7] << 24));
    for (int i = 0; i < 16; ++i) {
        uint32_t idx = (indices >> (2 * i)) & 0x3u;
        out_rgba[i] = static_cast<uint32_t>(r[idx]) |
                      (static_cast<uint32_t>(g[idx]) << 8) |
                      (static_cast<uint32_t>(b[idx]) << 16) |
                      (static_cast<uint32_t>(a[idx]) << 24);
    }
}

inline void decode_dxt_slice_to_rgba8(
    const uint8_t* src, uint32_t width, uint32_t height, GLenum format, uint8_t* dst_rgba) noexcept {
    const uint32_t blocks_x = std::max<uint32_t>(1u, (width + 3u) / 4u);
    const uint32_t blocks_y = std::max<uint32_t>(1u, (height + 3u) / 4u);
    const bool is_dxt1 = (format == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
                          format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
                          format == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT);
    const bool is_dxt3 = (format == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT);
    const size_t block_bytes = is_dxt1 ? 8u : 16u;

    const uint8_t* ptr = src;
    for (uint32_t by = 0; by < blocks_y; ++by) {
        for (uint32_t bx = 0; bx < blocks_x; ++bx) {
            uint32_t pixels[16];
            if (is_dxt1) {
                decode_dxt_color_block(ptr, pixels, format != GL_COMPRESSED_RGB_S3TC_DXT1_EXT);
            } else if (is_dxt3) {
                decode_dxt_color_block(ptr + 8, pixels, false);
                uint64_t alpha_4bit = 0;
                std::memcpy(&alpha_4bit, ptr, 8);
                for (int i = 0; i < 16; ++i) {
                    uint8_t a4 = static_cast<uint8_t>((alpha_4bit >> (4 * i)) & 0xFu);
                    uint8_t a8 = static_cast<uint8_t>((a4 << 4) | a4);
                    pixels[i] = (pixels[i] & 0x00FFFFFFu) | (static_cast<uint32_t>(a8) << 24);
                }
            } else {
                // DXT5 interpolated alpha
                uint8_t alpha_pal[8];
                alpha_pal[0] = ptr[0];
                alpha_pal[1] = ptr[1];
                if (alpha_pal[0] > alpha_pal[1]) {
                    for (int i = 1; i < 7; ++i)
                        alpha_pal[i + 1] = static_cast<uint8_t>(((7 - i) * alpha_pal[0] + i * alpha_pal[1]) / 7);
                } else {
                    for (int i = 1; i < 5; ++i)
                        alpha_pal[i + 1] = static_cast<uint8_t>(((5 - i) * alpha_pal[0] + i * alpha_pal[1]) / 5);
                    alpha_pal[6] = 0;
                    alpha_pal[7] = 255;
                }
                decode_dxt_color_block(ptr + 8, pixels, false);
                uint64_t alpha_bits = 0;
                for (int i = 0; i < 6; ++i) {
                    alpha_bits |= (static_cast<uint64_t>(ptr[2 + i]) << (8 * i));
                }
                for (int i = 0; i < 16; ++i) {
                    uint8_t a = alpha_pal[(alpha_bits >> (3 * i)) & 0x7u];
                    pixels[i] = (pixels[i] & 0x00FFFFFFu) | (static_cast<uint32_t>(a) << 24);
                }
            }

            for (uint32_t py = 0; py < 4; ++py) {
                uint32_t dst_y = by * 4 + py;
                if (dst_y >= height) continue;
                for (uint32_t px = 0; px < 4; ++px) {
                    uint32_t dst_x = bx * 4 + px;
                    if (dst_x >= width) continue;
                    size_t dst_idx = (static_cast<size_t>(dst_y) * width + dst_x) * 4u;
                    std::memcpy(dst_rgba + dst_idx, &pixels[py * 4 + px], 4u);
                }
            }
            ptr += block_bytes;
        }
    }
}

// Rewrites Project Alice's desktop GLSL 460 preamble & implicit int-to-float idioms
// into standards-compliant GLSL ES 3.20 (#version 320 es).
inline std::string transpile_glsl460_to_gles320(std::string_view raw) {
    std::string out;
    out.reserve(raw.size() + 512);

    // Strip desktop #version 460 and GL_ARB_* lines
    size_t pos = 0;
    bool injected_header = false;
    auto inject_es32_header = [&]() {
        if (injected_header) return;
        injected_header = true;
        out +=
            "#version 320 es\n"
            "precision highp float;\n"
            "precision highp int;\n"
            "precision highp sampler2D;\n"
            "precision highp sampler2DArray;\n"
            "precision highp usampler2D;\n"
            "float max(float a, int b) { return max(a, float(b)); }\n"
            "float max(int a, float b) { return max(float(a), b); }\n"
            "float min(float a, int b) { return min(a, float(b)); }\n"
            "float min(int a, float b) { return min(float(a), b); }\n";
    };

    while (pos < raw.size()) {
        size_t eol = raw.find('\n', pos);
        std::string_view line = (eol == std::string_view::npos)
                                    ? raw.substr(pos)
                                    : raw.substr(pos, eol - pos + 1);
        pos = (eol == std::string_view::npos) ? raw.size() : (eol + 1);

        size_t first = line.find_first_not_of(" \t\r");
        if (first != std::string_view::npos) {
            std::string_view trimmed = line.substr(first);
            if (trimmed.starts_with("#version ")) {
                inject_es32_header();
                continue;
            }
            if (trimmed.starts_with("#extension GL_ARB_")) {
                continue;
            }
        }
        out.append(line);
    }

    if (!injected_header) {
        std::string prefixed;
        prefixed.reserve(out.size() + 384);
        std::swap(out, prefixed);
        inject_es32_header();
        out += prefixed;
    }

    // Patch known desktop implicit int->float expressions in Alice's GLSL shaders
    auto replace_all = [&](std::string_view from, std::string_view to) {
        size_t p = 0;
        while ((p = out.find(from, p)) != std::string::npos) {
            out.replace(p, from.size(), to);
            p += to.size();
        }
    };
    replace_all("1 / map_size.", "1.0 / map_size.");
    replace_all("(1 - 1 / 1.3) * 3 / 5", "(1.0 - 1.0 / 1.3) * 3.0 / 5.0");
    replace_all("corrected_coord.y > 1", "corrected_coord.y > 1.0");
    replace_all("corrected_coord.y < 0", "corrected_coord.y < 0.0");
    replace_all("tex_coord * 3 ", "tex_coord * 3.0 ");
    replace_all("tex_coord * 1 ", "tex_coord * 1.0 ");
    replace_all("tex_coord * 2 ", "tex_coord * 2.0 ");
    replace_all("tex_coord * 5 ", "tex_coord * 5.0 ");
    replace_all("(vBumpTex / 2)", "(vBumpTex / 2.0)");
    replace_all("2 * far * near", "2.0 * far * near");
    replace_all("PI / 6", "PI / 6.0");
    replace_all("subroutines_index_2 == 0", "subroutines_index_2 == 0u");
    replace_all("2 * sign(rel_coord.y) + 3", "2.0 * sign(rel_coord.y) + 3.0");
    replace_all(": 0;", ": 0.0;");

    return out;
}

} // namespace vic2_mobile::gles

#ifdef glShaderSource
#undef glShaderSource
#endif
#ifdef glCompressedTexImage2D
#undef glCompressedTexImage2D
#endif
#ifdef glCompressedTexImage3D
#undef glCompressedTexImage3D
#endif

void vic2_glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) noexcept {
    std::string combined;
    for (GLsizei i = 0; i < count; ++i) {
        if (!string[i]) continue;
        if (length && length[i] >= 0) {
            combined.append(string[i], static_cast<size_t>(length[i]));
        } else {
            combined.append(string[i]);
        }
        combined.push_back('\n');
    }
    std::string es_src = vic2_mobile::gles::transpile_glsl460_to_gles320(combined);
    const GLchar* ptr = es_src.c_str();
    GLint len = static_cast<GLint>(es_src.size());
    ::glShaderSource(shader, 1, &ptr, &len);
}

void vic2_glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
                                 GLsizei width, GLsizei height, GLint border,
                                 GLsizei imageSize, const void* data) noexcept {
    const bool is_s3tc = (internalformat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
                          internalformat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
                          internalformat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
                          internalformat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ||
                          internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT ||
                          internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT);
    if (!is_s3tc || vic2_mobile::gles::supports_hardware_s3tc() || data == nullptr) {
        ::glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
        return;
    }

    std::vector<uint8_t> rgba(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u, 0);
    vic2_mobile::gles::decode_dxt_slice_to_rgba8(
        static_cast<const uint8_t*>(data),
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        internalformat,
        rgba.data());
    glTexImage2D(target, level, GL_RGBA8, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
}

void vic2_glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat,
                                 GLsizei width, GLsizei height, GLsizei depth, GLint border,
                                 GLsizei imageSize, const void* data) noexcept {
    const bool is_s3tc = (internalformat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
                          internalformat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
                          internalformat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
                          internalformat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ||
                          internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT ||
                          internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT);
    if (!is_s3tc || vic2_mobile::gles::supports_hardware_s3tc() || data == nullptr) {
        ::glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
        return;
    }

    const bool is_dxt1 = (internalformat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
                          internalformat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
                          internalformat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT);
    const size_t block_bytes = is_dxt1 ? 8u : 16u;
    const size_t blocks_x = std::max<size_t>(1u, (static_cast<size_t>(width) + 3u) / 4u);
    const size_t blocks_y = std::max<size_t>(1u, (static_cast<size_t>(height) + 3u) / 4u);
    const size_t slice_compressed_bytes = blocks_x * blocks_y * block_bytes;
    const size_t slice_rgba_bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;

    std::vector<uint8_t> rgba_all(slice_rgba_bytes * static_cast<size_t>(depth), 0);
    const auto* src_base = static_cast<const uint8_t*>(data);

    for (GLsizei z = 0; z < depth; ++z) {
        size_t src_off = static_cast<size_t>(z) * slice_compressed_bytes;
        if (src_off + slice_compressed_bytes > static_cast<size_t>(imageSize)) break;
        vic2_mobile::gles::decode_dxt_slice_to_rgba8(
            src_base + src_off,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            internalformat,
            rgba_all.data() + static_cast<size_t>(z) * slice_rgba_bytes);
    }

    glTexImage3D(target, level, GL_RGBA8, width, height, depth, border, GL_RGBA, GL_UNSIGNED_BYTE, rgba_all.data());
}

#define glShaderSource           vic2_glShaderSource
#define glCompressedTexImage2D   vic2_glCompressedTexImage2D
#define glCompressedTexImage3D   vic2_glCompressedTexImage3D

namespace ogl {

static SDL_GLContext g_sdl_gl_context = nullptr;

void create_opengl_context(sys::state& state) {
    auto* sdl_win = reinterpret_cast<SDL_Window*>(state.win_ptr ? state.win_ptr->window : nullptr);
    if (!sdl_win) {
        window::emit_error_message("SDL_Window is null during ogl::create_opengl_context", true);
        return;
    }

    g_sdl_gl_context = SDL_GL_CreateContext(sdl_win);
    if (!g_sdl_gl_context) {
        // Fallback from GLES 3.2 to GLES 3.0 if emulator/driver only advertises 3.0/3.1
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        g_sdl_gl_context = SDL_GL_CreateContext(sdl_win);
    }
    if (!g_sdl_gl_context) {
        window::emit_error_message(std::string("Failed to create OpenGL ES context: ") + SDL_GetError(), true);
        return;
    }

    SDL_GL_MakeCurrent(sdl_win, g_sdl_gl_context);
    SDL_GL_SetSwapInterval(1);
    (void)vic2_mobile::gles::supports_hardware_s3tc();
}

void shutdown_opengl(sys::state& /*state*/) {
    if (g_sdl_gl_context) {
        SDL_GL_DeleteContext(g_sdl_gl_context);
        g_sdl_gl_context = nullptr;
    }
}

} // namespace ogl
