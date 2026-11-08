#pragma once
// =============================================================================
// tools/headless_sim/include/glew.h
// Complete Headless OpenGL / GLEW Mock Engine
// Allows full compilation of map, texture, shaders, and UI without GPU / display.
// =============================================================================

#include <cstdint>
#include <cstddef>

#ifndef GL_ZERO
#define GL_ZERO 0
#define GL_NONE 0
#define GL_FALSE 0
#define GL_TRUE 1
#endif

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef int8_t GLbyte;
typedef int16_t GLshort;
typedef int32_t GLint;
typedef int32_t GLsizei;
typedef uint8_t GLubyte;
typedef uint16_t GLushort;
typedef uint32_t GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef struct __GLsync* GLsync;
typedef void (*GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

// Buffers and Drawing
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_STREAM_DRAW 0x88E0
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_LINES 0x0001
#define GL_LINE_STRIP 0x0003
#define GL_POINTS 0x0000
#define GL_LINE_SMOOTH 0x0B20
#define GL_PRIMITIVE_RESTART 0x8F9D

// Blending and Tests
#define GL_BLEND 0x0BE2
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE 1
#define GL_FUNC_ADD 0x8006
#define GL_DEPTH_TEST 0x0B71
#define GL_STENCIL_TEST 0x0B90
#define GL_SCISSOR_TEST 0x0C11
#define GL_CULL_FACE 0x0B44
#define GL_BACK 0x0405
#define GL_FRONT 0x0404
#define GL_LESS 0x0201
#define GL_GREATER 0x0204
#define GL_MULTISAMPLE 0x809D
#define GL_FIRST_VERTEX_CONVENTION 0x8E4D

// Textures and Formats
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#define GL_TEXTURE_BUFFER 0x8C2A
#define GL_RGBA 0x1908
#define GL_RGB 0x1907
#define GL_RGBA8 0x8058
#define GL_RGB8 0x8051
#define GL_SRGB 0x8C40
#define GL_SRGB_ALPHA 0x8C42
#define GL_RED 0x1903
#define GL_RED_INTEGER 0x8D94
#define GL_RG 0x8227
#define GL_RG8 0x822B
#define GL_RG32F 0x8230
#define GL_RG32I 0x823B
#define GL_R8 0x8229
#define GL_R8UI 0x8232
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_FLOAT 0x1406
#define GL_INT 0x1404
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_UNPACK_IMAGE_HEIGHT 0x806E
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_CLAMP_TO_BORDER 0x812D

// Framebuffers & Renderbuffers
#define GL_FRAMEBUFFER 0x8D40
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_UNDEFINED 0x8219
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA7

// Shaders
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84

// Texture Units
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6
#define GL_TEXTURE7 0x84C7
#define GL_TEXTURE8 0x84C8
#define GL_TEXTURE9 0x84C9
#define GL_TEXTURE10 0x84CA
#define GL_TEXTURE11 0x84CB
#define GL_TEXTURE12 0x84CC
#define GL_TEXTURE13 0x84CD
#define GL_TEXTURE14 0x84CE
#define GL_TEXTURE15 0x84CF
#define GL_TEXTURE16 0x84D0
#define GL_TEXTURE17 0x84D1

// Debug / Misc
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_DEBUG_SEVERITY_LOW 0x9148
#define GL_DEBUG_TYPE_OTHER 0x8251
#define GL_DONT_CARE 0x1100
#define GL_TIME_ELAPSED 0x88BF
#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

// Errors
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_OUT_OF_MEMORY 0x0505
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506

// -----------------------------------------------------------------------------
// Stub Functions
// -----------------------------------------------------------------------------
inline GLenum glGetError() { return GL_NO_ERROR; }

inline void glGenTextures(GLsizei n, GLuint* textures) {
    static GLuint s_id = 1;
    for (GLsizei i = 0; i < n; ++i) textures[i] = s_id++;
}
inline void glBindTexture(GLenum, GLuint) {}
inline void glTexParameteri(GLenum, GLenum, GLint) {}
inline void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {}
inline void glTexImage2DMultisample(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLboolean) {}
inline void glTexImage3D(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*) {}
inline void glCompressedTexImage2D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const void*) {}
inline void glCompressedTexImage3D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const void*) {}
inline void glPixelStorei(GLenum, GLint) {}
inline void glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*) {}
inline void glTexSubImage3D(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void*) {}
inline void glTexStorage2D(GLenum, GLsizei, GLenum, GLsizei, GLsizei) {}
inline void glTexStorage3D(GLenum, GLsizei, GLenum, GLsizei, GLsizei, GLsizei) {}
inline void glTexBuffer(GLenum, GLenum, GLuint) {}
inline void glGetTextureImage(GLuint, GLint, GLenum, GLenum, GLsizei, void*) {}
inline void glDeleteTextures(GLsizei, const GLuint*) {}
inline void glActiveTexture(GLenum) {}
inline void glGenerateMipmap(GLenum) {}

inline void glGenBuffers(GLsizei n, GLuint* buffers) {
    static GLuint s_buf_id = 1;
    for (GLsizei i = 0; i < n; ++i) buffers[i] = s_buf_id++;
}
inline void glBindBuffer(GLenum, GLuint) {}
inline void glBufferData(GLenum, GLsizeiptr, const void*, GLenum) {}
inline void glBufferSubData(GLenum, GLintptr, GLsizeiptr, const void*) {}
inline void glBufferStorage(GLenum, GLsizeiptr, const void*, GLbitfield) {}
inline void glDeleteBuffers(GLsizei, const GLuint*) {}

inline void glGenVertexArrays(GLsizei n, GLuint* arrays) {
    static GLuint s_vao_id = 1;
    for (GLsizei i = 0; i < n; ++i) arrays[i] = s_vao_id++;
}
inline void glBindVertexArray(GLuint) {}
inline void glDeleteVertexArrays(GLsizei, const GLuint*) {}
inline void glEnableVertexAttribArray(GLuint) {}
inline void glDisableVertexAttribArray(GLuint) {}
inline void glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*) {}
inline void glVertexAttribFormat(GLuint, GLint, GLenum, GLboolean, GLuint) {}
inline void glVertexAttribIFormat(GLuint, GLint, GLenum, GLuint) {}
inline void glVertexAttribBinding(GLuint, GLuint) {}
inline void glBindVertexBuffer(GLuint, GLuint, GLintptr, GLsizei) {}

inline void glGenFramebuffers(GLsizei n, GLuint* framebuffers) {
    static GLuint s_fb_id = 1;
    for (GLsizei i = 0; i < n; ++i) framebuffers[i] = s_fb_id++;
}
inline void glBindFramebuffer(GLenum, GLuint) {}
inline void glFramebufferTexture(GLenum, GLenum, GLuint, GLint) {}
inline void glFramebufferTexture2D(GLenum, GLenum, GLenum, GLuint, GLint) {}
inline void glFramebufferTextureLayer(GLenum, GLenum, GLuint, GLint, GLint) {}
inline void glFramebufferRenderbuffer(GLenum, GLenum, GLenum, GLuint) {}
inline GLenum glCheckFramebufferStatus(GLenum) { return GL_FRAMEBUFFER_COMPLETE; }
inline void glDeleteFramebuffers(GLsizei, const GLuint*) {}
inline void glBlitFramebuffer(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum) {}
inline void glDrawBuffers(GLsizei, const GLenum*) {}

inline void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
    static GLuint s_rb_id = 1;
    for (GLsizei i = 0; i < n; ++i) renderbuffers[i] = s_rb_id++;
}
inline void glBindRenderbuffer(GLenum, GLuint) {}
inline void glRenderbufferStorage(GLenum, GLenum, GLsizei, GLsizei) {}
inline void glRenderbufferStorageMultisample(GLenum, GLsizei, GLenum, GLsizei, GLsizei) {}
inline void glDeleteRenderbuffers(GLsizei, const GLuint*) {}

inline GLuint glCreateShader(GLenum) { static GLuint s_sh = 1; return s_sh++; }
inline void glShaderSource(GLuint, GLsizei, const GLchar* const*, const GLint*) {}
inline void glCompileShader(GLuint) {}
inline void glGetShaderiv(GLuint, GLenum pname, GLint* params) {
    if (pname == GL_COMPILE_STATUS && params) *params = GL_TRUE;
    else if (pname == GL_INFO_LOG_LENGTH && params) *params = 0;
}
inline void glGetShaderInfoLog(GLuint, GLsizei, GLsizei* length, GLchar*) { if (length) *length = 0; }
inline void glDeleteShader(GLuint) {}

inline GLuint glCreateProgram() { static GLuint s_prg = 1; return s_prg++; }
inline void glAttachShader(GLuint, GLuint) {}
inline void glLinkProgram(GLuint) {}
inline void glGetProgramiv(GLuint, GLenum pname, GLint* params) {
    if (pname == GL_LINK_STATUS && params) *params = GL_TRUE;
    else if (pname == GL_INFO_LOG_LENGTH && params) *params = 0;
}
inline void glGetProgramInfoLog(GLuint, GLsizei, GLsizei* length, GLchar*) { if (length) *length = 0; }
inline void glUseProgram(GLuint) {}
inline void glDeleteProgram(GLuint) {}

inline GLint glGetUniformLocation(GLuint, const GLchar*) { return 0; }
inline void glUniform1i(GLint, GLint) {}
inline void glUniform1f(GLint, GLfloat) {}
inline void glUniform1ui(GLint, GLuint) {}
inline void glUniform2f(GLint, GLfloat, GLfloat) {}
inline void glUniform2ui(GLint, GLuint, GLuint) {}
inline void glUniform3f(GLint, GLfloat, GLfloat, GLfloat) {}
inline void glUniform4f(GLint, GLfloat, GLfloat, GLfloat, GLfloat) {}
inline void glUniformMatrix3fv(GLint, GLsizei, GLboolean, const GLfloat*) {}
inline void glUniformSubroutinesuiv(GLenum, GLsizei, const GLuint*) {}

inline void glEnable(GLenum) {}
inline void glDisable(GLenum) {}
inline void glBlendFunc(GLenum, GLenum) {}
inline void glBlendFuncSeparate(GLenum, GLenum, GLenum, GLenum) {}
inline void glBlendEquation(GLenum) {}
inline void glCullFace(GLenum) {}
inline void glDepthFunc(GLenum) {}
inline void glDepthRange(GLdouble, GLdouble) {}
inline void glViewport(GLint, GLint, GLsizei, GLsizei) {}
inline void glScissor(GLint, GLint, GLsizei, GLsizei) {}
inline void glLineWidth(GLfloat) {}
inline void glClear(GLbitfield) {}
inline void glClearColor(GLfloat, GLfloat, GLfloat, GLfloat) {}
inline void glClearDepth(GLdouble) {}
inline void glClearTexImage(GLuint, GLint, GLenum, GLenum, const void*) {}
inline void glDrawArrays(GLenum, GLint, GLsizei) {}
inline void glDrawElements(GLenum, GLsizei, GLenum, const void*) {}
inline void glMultiDrawArrays(GLenum, const GLint*, const GLsizei*, GLsizei) {}
inline void glMultiDrawArraysIndirect(GLenum, const void*, GLsizei, GLsizei) {}
inline void glProvokingVertex(GLenum) {}
inline void glPrimitiveRestartIndex(GLuint) {}

inline void glGenQueries(GLsizei n, GLuint* ids) {
    static GLuint s_qid = 1;
    for (GLsizei i = 0; i < n; ++i) ids[i] = s_qid++;
}
inline void glDeleteQueries(GLsizei, const GLuint*) {}
inline void glBeginQuery(GLenum, GLuint) {}
inline void glEndQuery(GLenum) {}
inline void glGetQueryObjectiv(GLuint, GLenum pname, GLint* params) {
    if (params) *params = (pname == GL_QUERY_RESULT_AVAILABLE ? GL_TRUE : 0);
}
inline void glGetQueryObjectuiv(GLuint, GLenum pname, GLuint* params) {
    if (params) *params = (pname == GL_QUERY_RESULT_AVAILABLE ? GL_TRUE : 0);
}

inline void glDebugMessageCallback(GLDEBUGPROC, const void*) {}
inline void glDebugMessageControl(GLenum, GLenum, GLenum, GLsizei, const GLuint*, GLboolean) {}
