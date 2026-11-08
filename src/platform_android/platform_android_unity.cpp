// =============================================================================
// platform_android_unity.cpp
// Unified Android Platform Abstraction Layer (PAL) & JNI Bridge:
//   - UAX #14 CJK + Kinsoku Shori Line Breaker & UAX #9 BiDi (replaces ICU)
//   - POSIX mmap + Caseless File Lookup + App-Private Sandbox FS
//   - OpenGL ES 3.2 + GLSL 460 -> 320 es Transpiler + DXT1/3/5 -> RGBA8 Decoder
//   - miniaudio AAudio / OpenSL ES Non-Fatal Audio Engine
//   - SDL2 Multi-Touch Gesture State Machine + 15/60 FPS Power Throttler
//   - Big-Core Capped TBB + Low-RAM Single-Bookmark Streaming Scenario Builder
// =============================================================================

#include "unordered_dense.h"
#include "dcon_generated.hpp"
#include "dcon_generated_ids.hpp"
#include "container_types.hpp"
#include "runtime_config_android.hpp"

#include "icu_ubrk_ubidi_android.cpp"
#include "simple_fs_android.cpp"
#include "opengl_wrapper_android.cpp"
#include "sound_android.cpp"
#include "window_android.cpp"
#include "entry_point_android.cpp"

#if defined(__ANDROID__)
#include <jni.h>

extern "C" {

JNIEXPORT void JNICALL
Java_com_vic2mobile_app_NativeBridge_nativeSetRuntimePaths(
    JNIEnv* env, jclass /*clazz*/,
    jstring vic2_assets_root,
    jstring alice_assets_root,
    jstring cache_dir,
    jfloat density) {
    const char* vic2_cstr  = vic2_assets_root  ? env->GetStringUTFChars(vic2_assets_root, nullptr)  : nullptr;
    const char* alice_cstr = alice_assets_root ? env->GetStringUTFChars(alice_assets_root, nullptr) : nullptr;
    const char* cache_cstr = cache_dir         ? env->GetStringUTFChars(cache_dir, nullptr)         : nullptr;
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        vic2_mobile::runtime::config().vic2_assets_root  = vic2_cstr  ? vic2_cstr  : "";
        vic2_mobile::runtime::config().alice_assets_root = alice_cstr ? alice_cstr : "";
        vic2_mobile::runtime::config().cache_dir         = cache_cstr ? cache_cstr : "";
        vic2_mobile::runtime::config().display_density   = (density > 0.5f) ? density : 2.5f;
    }
    VIC2_LOGI("Configured paths: vic2='%s', alice='%s', cache='%s', density=%.2f",
              vic2_cstr ? vic2_cstr : "",
              alice_cstr ? alice_cstr : "",
              cache_cstr ? cache_cstr : "",
              density);
    if (vic2_cstr)  env->ReleaseStringUTFChars(vic2_assets_root, vic2_cstr);
    if (alice_cstr) env->ReleaseStringUTFChars(alice_assets_root, alice_cstr);
    if (cache_cstr) env->ReleaseStringUTFChars(cache_dir, cache_cstr);
}

JNIEXPORT void JNICALL
Java_com_vic2mobile_app_NativeBridge_nativeSetSafeAreaInsets(
    JNIEnv* /*env*/, jclass /*clazz*/, jint left, jint top, jint right, jint bottom) {
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        vic2_mobile::runtime::config().insets = {left, top, right, bottom};
    }
    VIC2_LOGI("Updated WindowInsets safe area: L=%d T=%d R=%d B=%d", left, top, right, bottom);
}

JNIEXPORT jint JNICALL
Java_com_vic2mobile_app_NativeBridge_nativeGetScenarioStage(JNIEnv* /*env*/, jclass /*clazz*/) {
    return vic2_mobile::runtime::config().scenario_build_stage.load(std::memory_order_acquire);
}

JNIEXPORT void JNICALL
Java_com_vic2mobile_app_NativeBridge_nativeSetInspectorMode(JNIEnv* /*env*/, jclass /*clazz*/, jboolean enabled) {
    vic2_mobile::input::global_touch_controller().set_inspector_mode(enabled == JNI_TRUE);
}

} // extern "C"
#endif
