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

#include "icu.h"
#include "simple_fs_win.cpp"
#include "icu_ubrk_ubidi_android.cpp"

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
    std::cerr << "[HEADLESS SIM " << (fatal ? "FATAL" : "WARN") << "] " << content << std::endl;
}

} // namespace window

// -----------------------------------------------------------------------------
// Audio / Sound Stubs
// -----------------------------------------------------------------------------
namespace sound {

audio_instance::~audio_instance() = default;
void audio_instance::play(float, bool, void*) {}
void audio_instance::pause() const {}
void audio_instance::resume() const {}
void audio_instance::stop() const {}
bool audio_instance::is_playing() const { return false; }
void audio_instance::change_volume(float) const {}

sound_impl::sound_impl() = default;
sound_impl::~sound_impl() = default;
void sound_impl::play_effect(audio_instance&, float) {}
void sound_impl::play_interface_sound(audio_instance&, float) {}
void sound_impl::play_music(int32_t, float) {}
void sound_impl::play_new_track(sys::state&) {}
void sound_impl::play_next_track(sys::state&) {}
void sound_impl::play_previous_track(sys::state&) {}

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
native_string get_current_track_name(sys::state&) { return NATIVE(""); }

static audio_instance dummy_audio;
audio_instance& get_click_sound(sys::state&) { return dummy_audio; }
audio_instance& get_click_left_sound(sys::state&) { return dummy_audio; }
audio_instance& get_click_right_sound(sys::state&) { return dummy_audio; }
audio_instance& get_tab_budget_sound(sys::state&) { return dummy_audio; }
audio_instance& get_hover_sound(sys::state&) { return dummy_audio; }
audio_instance& get_checkbox_sound(sys::state&) { return dummy_audio; }
audio_instance& get_enact_sound(sys::state&) { return dummy_audio; }
audio_instance& get_subtab_sound(sys::state&) { return dummy_audio; }
audio_instance& get_delete_sound(sys::state&) { return dummy_audio; }
audio_instance& get_autochoose_sound(sys::state&) { return dummy_audio; }
audio_instance& get_tab_politics_sound(sys::state&) { return dummy_audio; }
audio_instance& get_tab_diplomacy_sound(sys::state&) { return dummy_audio; }
audio_instance& get_tab_military_sound(sys::state&) { return dummy_audio; }
audio_instance& get_tab_population_sound(sys::state&) { return dummy_audio; }
audio_instance& get_tab_production_sound(sys::state&) { return dummy_audio; }
audio_instance& get_tab_technology_sound(sys::state&) { return dummy_audio; }

} // namespace sound

// -----------------------------------------------------------------------------
// OpenGL Stubs
// -----------------------------------------------------------------------------
namespace ogl {

void create_opengl_context(sys::state&) {}
void shutdown_opengl(sys::state&) {}
void initialize_msaa(sys::state&, int32_t, int32_t) {}
void deinitialize_msaa(sys::state&) {}
void initialize_framebuffer_for_province_indices(sys::state&, int32_t, int32_t) {}
void deinitialize_framebuffer_for_province_indices(sys::state&) {}

} // namespace ogl

// Minimal OpenGL API symbols referenced by system_state.cpp:
extern "C" {
void glGenQueries(GLsizei, GLuint*) {}
void glBeginQuery(GLenum, GLuint) {}
void glEndQuery(GLenum) {}
void glGetQueryObjectuiv(GLuint, GLenum, GLuint*) {}
void glClearColor(GLfloat, GLfloat, GLfloat, GLfloat) {}
void glClear(GLbitfield) {}
void glEnable(GLenum) {}
void glDisable(GLenum) {}
void glBlendFunc(GLenum, GLenum) {}
void glUseProgram(GLuint) {}
void glDeleteQueries(GLsizei, const GLuint*) {}
}
