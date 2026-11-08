// =============================================================================
// entry_point_android.cpp
// Android SDL_main Entry Point, Big-Core TBB Thread Cap (2~4 threads),
// Multi-Root Asset FS (Built-in Alice Assets + SAF Victoria 2 Directory), and
// Single-Bookmark (1836.1.1) Low-RAM (<1.2GB) Streaming .bin Scenario Builder.
// =============================================================================

#include "runtime_config_android.hpp"
#include "serialization.hpp"
#include "system_state.hpp"
#include "game_scene.hpp"
#include "parsers_declarations.hpp"

#include <SDL.h>
#include <oneapi/tbb/global_control.h>
#include <sys/stat.h>
#if defined(__ANDROID__) || defined(__linux__)
#include <dlfcn.h>
#endif

#include <algorithm>
#include <ctime>
#include <memory>
#include <thread>
#include <vector>

namespace {

struct scenario_file_entry {
    native_string file_name;
    sys::mod_identifier ident;
};

// Heap-allocated so DCON's large backing store is only allocated inside SDL_main
std::unique_ptr<sys::state> g_game_state;
std::vector<parsers::mod_file> g_mod_list;
std::vector<scenario_file_entry> g_scenario_files;
native_string g_selected_scenario_file;
uint32_t g_max_scenario_count = 0;

void configure_base_filesystem(simple_fs::file_system& fs) {
    simple_fs::reset(fs);
    std::string alice_root;
    std::string vic2_root;
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        alice_root = vic2_mobile::runtime::config().alice_assets_root;
        vic2_root  = vic2_mobile::runtime::config().vic2_assets_root;
    }

    // 1. Base root: Project Alice built-in assets/ (shaders, lua, svg, alice.gui, fonts)
    if (!alice_root.empty()) {
        simple_fs::add_root(fs, alice_root);
    }
    // 2. Game root: Victoria 2 data imported via SAF, local game_data/, or current working dir
    if (!vic2_root.empty()) {
        simple_fs::add_root(fs, vic2_root);
    } else {
        struct stat st {};
        if (stat("game_data", &st) == 0 && S_ISDIR(st.st_mode)) {
            simple_fs::add_root(fs, NATIVE("game_data"));
        } else {
            simple_fs::add_root(fs, NATIVE("."));
        }
    }
    // Re-add alice_root last ONLY if vic2_root lacks assets/, handled naturally since vic2_root has no assets/ dir
    // and simple_fs searches ordered_roots from back to front!
}

native_string produce_mod_path() {
    simple_fs::file_system dummy;
    configure_base_filesystem(dummy);
    for (auto& m : g_mod_list) {
        if (!m.mod_selected) break;
        m.add_to_file_system(dummy);
    }
    return simple_fs::extract_state(dummy);
}

void scan_mods_folder() {
    g_mod_list.clear();
    simple_fs::file_system fs_root;
    configure_base_filesystem(fs_root);
    auto root = simple_fs::get_root(fs_root);
    auto mod_dir = simple_fs::open_directory(root, NATIVE("mod"));
    auto mod_files = simple_fs::list_files(mod_dir, NATIVE(".mod"));
    parsers::error_handler err("");
    for (auto& f : mod_files) {
        if (auto of = simple_fs::open_file(f); of) {
            auto content = simple_fs::view_contents(*of);
            parsers::token_generator gen(content.data, content.data + content.file_size);
            g_mod_list.push_back(parsers::parse_mod_file(gen, err, parsers::mod_file_context{}));
        }
    }
}

void scan_scenario_folder() {
    g_scenario_files.clear();
    g_max_scenario_count = 0;
    auto sdir = simple_fs::get_or_create_scenario_directory();
    for (auto& f : simple_fs::list_files(sdir, NATIVE(".bin"))) {
        if (auto of = simple_fs::open_file(f); of) {
            auto content = simple_fs::view_contents(*of);
            auto desc = sys::extract_mod_information(reinterpret_cast<uint8_t const*>(content.data), content.file_size);
            if (desc.count != 0) {
                g_max_scenario_count = std::max(desc.count, g_max_scenario_count);
                g_scenario_files.push_back(scenario_file_entry{simple_fs::get_file_name(f), desc});
            }
        }
    }
    std::sort(g_scenario_files.begin(), g_scenario_files.end(), [](const auto& a, const auto& b) {
        return a.ident.count > b.ident.count;
    });
}

native_string to_hex_u64(uint64_t v) {
    native_string ret;
    constexpr native_char digits[] = NATIVE("0123456789ABCDEF");
    do {
        ret += digits[v & 0x0F];
        v >>= 4;
    } while (v != 0);
    return ret;
}

// Low-RAM Single-Bookmark Scenario Builder:
// Only builds bookmark index 0 (1836.1.1) inside a scoped temporary sys::state,
// flushes compressed .bin to disk, and immediately frees the temporary state
// BEFORE allocating the main runtime g_game_state (< 1.2GB peak RAM).
void build_mobile_scenario_file() {
    vic2_mobile::runtime::set_status(1, "Parsing Victoria II 1836 scenario files...");

    auto path = produce_mod_path();
    simple_fs::file_system fs_root;
    simple_fs::restore_state(fs_root, path);
    parsers::error_handler err("");
    auto root = simple_fs::get_root(fs_root);
    auto common = simple_fs::open_directory(root, NATIVE("common"));

    parsers::bookmark_context bookmark_context;
    if (auto f = simple_fs::open_file(common, NATIVE("bookmarks.txt")); f) {
        auto bookmark_content = simple_fs::view_contents(*f);
        err.file_name = "bookmarks.txt";
        parsers::token_generator gen(bookmark_content.data, bookmark_content.data + bookmark_content.file_size);
        parsers::parse_bookmark_file(gen, err, bookmark_context);
    } else {
        window::emit_error_message(
            "Could not open common/bookmarks.txt! Please verify your imported Victoria II folder.", true);
        return;
    }

    sys::year_month_day start_date{1836, 1, 1};
    if (!bookmark_context.bookmark_dates.empty()) {
        start_date = bookmark_context.bookmark_dates[0].date_;
    }

    {
        auto temp_builder_state = std::make_unique<sys::state>();
        simple_fs::restore_state(temp_builder_state->common_fs, path);
        temp_builder_state->load_scenario_data(err, start_date);

        if (err.fatal) {
            window::emit_error_message("Fatal error while building scenario:\n" + err.accumulated_errors, true);
            return;
        }

        vic2_mobile::runtime::set_status(2, "Serializing compressed .bin scenario cache...");
        auto sdir = simple_fs::get_or_create_scenario_directory();
        int32_t append = 0;
        auto base_name = to_hex_u64(static_cast<uint64_t>(std::time(nullptr)));
        while (simple_fs::peek_file(sdir, base_name + NATIVE("-") + std::to_string(append) + NATIVE(".bin"))) {
            ++append;
        }
        ++g_max_scenario_count;
        g_selected_scenario_file = base_name + NATIVE("-") + std::to_string(append) + NATIVE(".bin");
        sys::write_scenario_file(*temp_builder_state, g_selected_scenario_file, g_max_scenario_count);

        if (auto of = simple_fs::open_file(sdir, g_selected_scenario_file); of) {
            auto content = simple_fs::view_contents(*of);
            auto desc = sys::extract_mod_information(reinterpret_cast<uint8_t const*>(content.data), content.file_size);
            if (desc.count != 0) {
                g_scenario_files.push_back(scenario_file_entry{g_selected_scenario_file, desc});
            }
        }
        // temp_builder_state destroyed here -> releases temporary DCON pages before runtime load!
    }

    if (!err.accumulated_errors.empty() || !err.accumulated_warnings.empty()) {
        std::string log_msg = "Errors:\n" + err.accumulated_errors + "\nWarnings:\n" + err.accumulated_warnings;
        auto pdir = simple_fs::get_or_create_scenario_directory();
        simple_fs::write_file(pdir, NATIVE("scenario_errors.log"), log_msg.data(), static_cast<uint32_t>(log_msg.size()));
        VIC2_LOGW("Scenario built with non-fatal warnings (saved to scenario_errors.log).");
    }
}

void find_matching_scenario_file() {
    g_selected_scenario_file.clear();
    auto mod_path = produce_mod_path();
    for (const auto& f : g_scenario_files) {
        if (f.ident.mod_path == mod_path) {
            g_selected_scenario_file = f.file_name;
            return;
        }
    }
    // Fallback: use newest valid .bin if paths match after Android app update
    if (!g_scenario_files.empty()) {
        g_selected_scenario_file = g_scenario_files.front().file_name;
    }
}

} // namespace

extern "C" int SDL_main(int argc, char* argv[]) {
#if defined(__ANDROID__) || defined(__linux__)
    // Promote libvic2mobile.so symbols to RTLD_GLOBAL on Android Bionic so LuaJIT ffi.C resolves DCON_LUADLL_API
    dlopen("libvic2mobile.so", RTLD_NOW | RTLD_GLOBAL);
#endif
    // Cap oneTBB worker pool to 2~4 big cores on ARM big.LITTLE SoCs to avoid little-core spinlocks
    const unsigned hw_threads = std::max(2u, std::thread::hardware_concurrency());
    const size_t big_core_threads = static_cast<size_t>(std::clamp(hw_threads / 2u, 2u, 4u));
    oneapi::tbb::global_control tbb_thread_cap(
        oneapi::tbb::global_control::max_allowed_parallelism, big_core_threads);
    VIC2_LOGI("Vic2Mobile SDL_main started: hw_threads=%u, TBB big-core cap=%zu", hw_threads, big_core_threads);

    scan_mods_folder();
    scan_scenario_folder();
    find_matching_scenario_file();

    if (g_selected_scenario_file.empty()) {
        VIC2_LOGI("No cached .bin scenario found; building 1836 scenario cache...");
        build_mobile_scenario_file();
    } else {
        VIC2_LOGI("Found cached scenario file: %s", g_selected_scenario_file.c_str());
    }

    // Allocate primary runtime game_state AFTER temporary scenario builder has freed its memory
    g_game_state = std::make_unique<sys::state>();
    configure_base_filesystem(g_game_state->common_fs);

    if (!sys::try_read_scenario_and_save_file(*g_game_state, g_selected_scenario_file)) {
        // Cached .bin may be stale after schema upgrade -> rebuild once automatically
        VIC2_LOGW("Cached scenario %s incompatible with current engine schema; rebuilding...",
                  g_selected_scenario_file.c_str());
        build_mobile_scenario_file();
        if (!sys::try_read_scenario_and_save_file(*g_game_state, g_selected_scenario_file)) {
            window::emit_error_message("Scenario file could not be read even after rebuild.", true);
            return EXIT_FAILURE;
        }
    }

    g_game_state->loaded_scenario_file = g_selected_scenario_file;
    g_game_state->fill_unsaved_data();

    network::init(*g_game_state);
    g_game_state->load_user_settings();
    ui::populate_definitions_map(*g_game_state);

    std::thread update_thread([&]() {
        g_game_state->game_loop();
    });
    std::thread ui_cache_update([&]() {
        g_game_state->ui_cached_data.process_update(*g_game_state);
    });
    std::thread graphics_cache_update([&]() {
        g_game_state->map_state.update_cache(*g_game_state);
    });
    std::thread map_labels_update([&]() {
        g_game_state->map_state.update_map_labels(*g_game_state);
    });

    window::create_window(
        *g_game_state,
        window::creation_parameters{1920, 1080, window::window_state::maximized, true});
    g_game_state->quit_signaled.store(true, std::memory_order_release);

    update_thread.join();
    ui_cache_update.join();
    graphics_cache_update.join();
    map_labels_update.join();

    network::finish(*g_game_state, true);
    g_game_state.reset();
    return EXIT_SUCCESS;
}
