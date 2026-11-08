#pragma once
// =============================================================================
// tools/headless_sim/include/wglew.h
// Minimal WGLEW types for headless compilation
// =============================================================================

#include "glew.h"

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042

inline int glewInit() { return 0; }
inline int wglewIsSupported(const char*) { return 0; }
inline int wglSwapIntervalEXT(int) { return 0; }
