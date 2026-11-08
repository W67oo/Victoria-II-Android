#pragma once
// =============================================================================
// arm64_shim/glew.h
// Maps Project Alice's desktop OpenGL 4.x / GLEW calls to Android OpenGL ES 3.2
// =============================================================================

#include <GLES3/gl32.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3ext.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#ifndef GLEW_OK
#define GLEW_OK 0
#endif

inline GLenum glewInit() noexcept {
    return GLEW_OK;
}

// Desktop GL constants mapped to OpenGL ES 3.2 equivalents or safe tokens
#ifndef GL_CLAMP_TO_BORDER
#ifdef GL_CLAMP_TO_BORDER_EXT
#define GL_CLAMP_TO_BORDER GL_CLAMP_TO_BORDER_EXT
#else
#define GL_CLAMP_TO_BORDER 0x812D
#endif
#endif

#ifndef GL_BGRA
#ifdef GL_BGRA_EXT
#define GL_BGRA GL_BGRA_EXT
#else
#define GL_BGRA 0x80E1
#endif
#endif

#ifndef GL_PRIMITIVE_RESTART
#define GL_PRIMITIVE_RESTART GL_PRIMITIVE_RESTART_FIXED_INDEX
#endif

#ifndef GL_FIRST_VERTEX_CONVENTION
#define GL_FIRST_VERTEX_CONVENTION 0x8E4D
#endif

#ifndef GL_LINE_SMOOTH
#define GL_LINE_SMOOTH 0x0B20
#endif

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

#ifndef GL_STACK_OVERFLOW
#define GL_STACK_OVERFLOW 0x0503
#endif

#ifndef GL_STACK_UNDERFLOW
#define GL_STACK_UNDERFLOW 0x0504
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#endif

#ifndef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#endif

#ifndef GL_TIME_ELAPSED
#ifdef GL_TIME_ELAPSED_EXT
#define GL_TIME_ELAPSED GL_TIME_ELAPSED_EXT
#else
#define GL_TIME_ELAPSED 0x88BF
#endif
#endif

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif

// Implemented in opengl_wrapper_android.cpp:
// 1. Automatic GLSL 460 -> GLSL ES 3.20 shader transpilation
// 2. Hardware S3TC check + software DXT1/DXT3/DXT5 -> RGBA8 decompression fallback
void vic2_glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) noexcept;
void vic2_glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
                                 GLsizei width, GLsizei height, GLint border,
                                 GLsizei imageSize, const void* data) noexcept;
void vic2_glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat,
                                 GLsizei width, GLsizei height, GLsizei depth, GLint border,
                                 GLsizei imageSize, const void* data) noexcept;

#ifndef VIC2_GLES_WRAPPER_IMPL
#define glShaderSource           vic2_glShaderSource
#define glCompressedTexImage2D   vic2_glCompressedTexImage2D
#define glCompressedTexImage3D   vic2_glCompressedTexImage3D
#endif

// Desktop OpenGL -> OpenGL ES 3.2 inline compatibility adapters
inline void glClearDepth(double depth) noexcept {
    glClearDepthf(static_cast<float>(depth));
}

inline void glDepthRange(double zNear, double zFar) noexcept {
    glDepthRangef(static_cast<float>(zNear), static_cast<float>(zFar));
}

inline void glProvokingVertex(GLenum /*provokeMode*/) noexcept {}

inline void glPrimitiveRestartIndex(GLuint /*index*/) noexcept {}

inline void glUniformSubroutinesuiv(GLenum /*shadertype*/, GLsizei /*count*/, const GLuint* /*indices*/) noexcept {}

inline void glBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) noexcept {
    glBufferData(target, size, data, (flags != 0) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
}

inline void glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat,
                                    GLsizei width, GLsizei height, GLboolean fixedsamplelocations) noexcept {
    GLenum sized_fmt = internalformat;
    if (sized_fmt == GL_RGBA) {
        sized_fmt = GL_RGBA8;
    } else if (sized_fmt == GL_RGB) {
        sized_fmt = GL_RGB8;
    }
    glTexStorage2DMultisample(target, samples, sized_fmt, width, height, fixedsamplelocations);
}

inline void glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount) noexcept {
    for (GLsizei i = 0; i < drawcount; ++i) {
        if (count[i] > 0) {
            glDrawArrays(mode, first[i], count[i]);
        }
    }
}

inline void glMultiDrawArraysIndirect(GLenum mode, const void* indirect, GLsizei drawcount, GLsizei stride) noexcept {
    const GLsizei step = (stride != 0) ? stride : 16;
    const auto* base = static_cast<const uint8_t*>(indirect);
    for (GLsizei i = 0; i < drawcount; ++i) {
        glDrawArraysIndirect(mode, base + static_cast<ptrdiff_t>(i) * step);
    }
}

inline void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void* data) noexcept {
    if (texture == 0) return;
    GLint prev_tex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);
    glBindTexture(GL_TEXTURE_2D, texture);

    GLint w = 0, h = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &h);
    if (w > 0 && h > 0) {
        size_t bpp = (format == GL_RGBA) ? 4 : (format == GL_RGB ? 3 : (format == GL_RG ? 2 : 1));
        std::vector<uint8_t> clear_buf(static_cast<size_t>(w) * static_cast<size_t>(h) * bpp, 0);
        if (data != nullptr) {
            const auto* src = static_cast<const uint8_t*>(data);
            for (size_t px = 0, n = static_cast<size_t>(w) * static_cast<size_t>(h); px < n; ++px) {
                std::memcpy(clear_buf.data() + px * bpp, src, bpp);
            }
        }
        glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, w, h, format, type, clear_buf.data());
    }
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prev_tex));
}

inline void glGetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type,
                              GLsizei /*bufSize*/, void* pixels) noexcept {
    if (texture == 0 || pixels == nullptr) return;
    GLint prev_tex = 0, prev_read_fbo = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prev_read_fbo);

    glBindTexture(GL_TEXTURE_2D, texture);
    GLint w = 0, h = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &h);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prev_tex));

    if (w > 0 && h > 0) {
        GLuint fbo = 0;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, level);
        if (glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
            glReadPixels(0, 0, w, h, format, type, pixels);
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prev_read_fbo));
        glDeleteFramebuffers(1, &fbo);
    }
}

inline bool vic2_has_disjoint_timer_query() noexcept {
    static int cached = -1;
    if (cached < 0) {
        const char* exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        cached = (exts && std::strstr(exts, "GL_EXT_disjoint_timer_query") != nullptr) ? 1 : 0;
    }
    return cached == 1;
}

inline void glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params) noexcept {
    if (!params) return;
    if (!vic2_has_disjoint_timer_query()) {
        *params = (pname == GL_QUERY_RESULT_AVAILABLE) ? GL_TRUE : 0;
        return;
    }
    GLuint uval = 0;
    glGetQueryObjectuiv(id, pname, &uval);
    *params = static_cast<GLint>(uval);
}

inline void vic2_glEnable(GLenum cap) noexcept {
    if (cap == GL_LINE_SMOOTH || cap == GL_MULTISAMPLE) {
        return;
    }
    glEnable(cap);
}
#ifndef VIC2_GLES_WRAPPER_IMPL
#define glEnable(cap) vic2_glEnable(cap)
#endif

inline void vic2_glBeginQuery(GLenum target, GLuint id) noexcept {
    if (target == GL_TIME_ELAPSED && !vic2_has_disjoint_timer_query()) {
        return;
    }
    glBeginQuery(target, id);
}
inline void vic2_glEndQuery(GLenum target) noexcept {
    if (target == GL_TIME_ELAPSED && !vic2_has_disjoint_timer_query()) {
        return;
    }
    glEndQuery(target);
}
#ifndef VIC2_GLES_WRAPPER_IMPL
#define glBeginQuery(target, id) vic2_glBeginQuery(target, id)
#define glEndQuery(target)       vic2_glEndQuery(target)
#endif
