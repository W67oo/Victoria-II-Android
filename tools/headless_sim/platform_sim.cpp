// =============================================================================
// platform_sim.cpp
// Headless Simulation Platform Support for Host Windows (x86_64)
// Provides Win32 filesystem, portable UAX #14 text breaking, and
// lightweight stubs for windowing, audio, and OpenGL.
// =============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

#include "system_state.hpp"
#include "icu.h"
#include "simple_fs_win.cpp"
#include "../../src/platform_android/icu_ubrk_ubidi_android.cpp"

#include "window.hpp"
#include "sound.hpp"
#include "opengl_wrapper.hpp"

// -----------------------------------------------------------------------------
// Windowing Stubs
// -----------------------------------------------------------------------------
namespace window {

win32_text_services::win32_text_services() = default;
win32_text_services::~win32_text_services() = default;
void win32_text_services::start_text_services() {}
void win32_text_services::end_text_services() {}
text_services_object* win32_text_services::create_text_service_object(sys::state&, ui::element_base&) { return nullptr; }
void win32_text_services::on_text_change(text_services_object*, uint32_t, uint32_t, uint32_t) {}
void win32_text_services::on_selection_change(text_services_object*) {}
void win32_text_services::set_focus(sys::state&, text_services_object*) {}
void win32_text_services::suspend_keystroke_handling() {}
void win32_text_services::resume_keystroke_handling() {}
bool win32_text_services::send_mouse_event_to_tso(text_services_object*, int32_t, int32_t, uint32_t) { return false; }

void create_window(sys::state&, creation_parameters const&) {}
void close_window(sys::state&) {}
void set_borderless_full_screen(sys::state&, bool) {}
bool is_in_fullscreen(sys::state const&) { return false; }
bool is_key_depressed(sys::state const&, sys::virtual_key) { return false; }
void change_cursor(sys::state&, cursor_type) {}
void get_window_size(sys::state const&, int& width, int& height) { width = 1920; height = 1080; }
int32_t cursor_blink_ms() { return 500; }
int32_t double_click_ms() { return 300; }
void release_text_services_object(text_services_object*) {}

void emit_error_message(std::string const& content, bool fatal) {
    if (fatal) {
        std::cerr << "[FATAL] " << content << std::endl;
    } else {
        std::cerr << "[WARN]  " << content << std::endl;
    }
}

void emit_error_message(std::string_view content, bool fatal) {
    emit_error_message(std::string(content), fatal);
}

} // namespace window

// -----------------------------------------------------------------------------
// Sound Engine Stubs
// -----------------------------------------------------------------------------
namespace sound {

audio_instance::~audio_instance() = default;
void audio_instance::play(float, bool, void*) {}
void audio_instance::pause() const {}
void audio_instance::resume() const {}
void audio_instance::stop() const {}
bool audio_instance::is_playing() const { return false; }
void audio_instance::change_volume(float) const {}

void initialize_sound_system(sys::state&) {}
void change_effect_volume(sys::state&, float) {}
void change_interface_volume(sys::state&, float) {}
void change_music_volume(sys::state&, float) {}
void play_effect(sys::state&, audio_instance&, float) {}
void play_interface_sound(sys::state&, audio_instance&, float) {}
void stop_music(sys::state&) {}
void start_music(sys::state&, float) {}
void pause_all(sys::state&) {}
void resume_all(sys::state&) {}
void update_music_track(sys::state&) {}
void play_new_track(sys::state&) {}
void play_next_track(sys::state&) {}
void play_previous_track(sys::state&) {}
native_string get_current_track_name(sys::state&) { return native_string(); }

static audio_instance s_dummy_audio;
audio_instance& get_click_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_click_left_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_click_right_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_tab_budget_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_hover_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_checkbox_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_enact_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_subtab_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_delete_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_autochoose_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_tab_politics_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_tab_diplomacy_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_tab_military_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_tab_population_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_tab_production_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_tab_technology_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_army_select_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_army_move_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_navy_select_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_navy_move_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_error_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_peace_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_army_built_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_navy_built_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_declaration_of_war_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_technology_finished_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_factory_built_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_election_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_revolt_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_fort_built_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_railroad_built_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_naval_base_built_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_minor_event_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_major_event_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_decline_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_accept_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_diplomatic_request_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_chat_message_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_console_open_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_console_close_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_event_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_decision_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_pause_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_unpause_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_enemycapitulated_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_wecapitulated_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_random_land_battle_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_random_naval_battle_sound(sys::state&) { return s_dummy_audio; }
audio_instance& get_random_province_select_sound(sys::state&) { return s_dummy_audio; }

} // namespace sound

namespace ogl {
void create_opengl_context(sys::state&) {}
} // namespace ogl
