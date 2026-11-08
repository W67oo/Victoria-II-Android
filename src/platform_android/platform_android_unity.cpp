// =============================================================================
// platform_android_unity.cpp
// NativeActivity Android Platform Layer (C++20, GLES 3.2, oneTBB, miniaudio)
// =============================================================================

#include "unordered_dense.h"
#include "dcon_generated.hpp"
#include "dcon_generated_ids.hpp"
#include "container_types.hpp"
#include "android_logger.hpp"

#include "../engine/filesystem/simple_fs_nix.cpp"
#include "opengl_wrapper_android.cpp"
#include "sound_android.cpp"
#include "window_android.cpp"
#include "entry_point_android.cpp"
