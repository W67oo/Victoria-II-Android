#pragma once
// =============================================================================
// runtime_config_android.hpp
// Shared Android runtime paths (SAF-imported Vic2 root, Alice built-in assets,
// App-private cache directory) and WindowInsets camera cutout safe area.
// =============================================================================

#include "touch_gesture_controller.hpp"

#include <atomic>
#include <mutex>
#include <string>

#if defined(__ANDROID__)
#include <android/log.h>
#define VIC2_LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "Vic2Mobile", __VA_ARGS__)
#define VIC2_LOGW(...) __android_log_print(ANDROID_LOG_WARN,  "Vic2Mobile", __VA_ARGS__)
#define VIC2_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "Vic2Mobile", __VA_ARGS__)
#else
#include <cstdio>
#define VIC2_LOGI(...) do { std::fprintf(stdout, __VA_ARGS__); std::fprintf(stdout, "\n"); } while(0)
#define VIC2_LOGW(...) do { std::fprintf(stdout, __VA_ARGS__); std::fprintf(stdout, "\n"); } while(0)
#define VIC2_LOGE(...) do { std::fprintf(stderr, __VA_ARGS__); std::fprintf(stderr, "\n"); } while(0)
#endif

namespace vic2_mobile::runtime {

struct AndroidRuntimeConfig {
    mutable std::mutex mtx;
    std::string vic2_assets_root;    // Extracted Victoria 2 game directory
    std::string alice_assets_root;   // Extracted Project Alice built-in assets parent dir
    std::string cache_dir;           // App-private data dir (scenarios, saves, settings, snapshots)
    input::SafeAreaInsets insets{};  // Camera cutout & gesture bar insets in physical pixels
    float display_density = 2.5f;
    std::atomic<int> scenario_build_stage{0}; // 0=idle, 1=parsing, 2=writing .bin, 3=ready, -1=error
    std::string last_status_message;

    std::string get_storage_root() const {
        std::lock_guard<std::mutex> lk(mtx);
        if (!cache_dir.empty()) {
            if (cache_dir.back() == '/') return cache_dir + "AliceData/";
            return cache_dir + "/AliceData/";
        }
        return "./AliceData/";
    }
};

inline AndroidRuntimeConfig& config() {
    static AndroidRuntimeConfig instance;
    return instance;
}

inline void set_status(int stage, std::string msg) {
    auto& cfg = config();
    cfg.scenario_build_stage.store(stage, std::memory_order_release);
    std::lock_guard<std::mutex> lk(cfg.mtx);
    cfg.last_status_message = std::move(msg);
    VIC2_LOGI("[Stage %d] %s", stage, cfg.last_status_message.c_str());
}

} // namespace vic2_mobile::runtime
