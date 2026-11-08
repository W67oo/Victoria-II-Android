// =============================================================================
// window_android.cpp
// SDL2 Window, Multi-Touch Gesture Dispatcher, Soft-Keyboard Bridge, Lifecycle
// Instant Snapshot Save & 15/60 FPS Dynamic Idle Power Throttler.
// =============================================================================

#include "runtime_config_android.hpp"
#include "window.hpp"
#include "map.hpp"
#include "gui_console.hpp"
#include "serialization.hpp"
#include "system_state.hpp"
#include "user_interactions.hpp"

#include <SDL.h>

#include <atomic>
#include <cmath>
#include <cstdlib>

namespace vic2_mobile::input {

inline TouchGestureController& global_touch_controller() noexcept {
    static TouchGestureController instance;
    return instance;
}

} // namespace vic2_mobile::input

namespace window {

namespace {

std::atomic<bool> g_window_should_close{false};
bool g_map_pan_active = false;
bool g_ui_drag_active = false;

sys::virtual_key sdl_keycode_to_virtual_key(SDL_Keycode key) noexcept {
    switch (key) {
    case SDLK_SPACE:        return sys::virtual_key::SPACE;
    case SDLK_QUOTE:        return sys::virtual_key::QUOTE;
    case SDLK_COMMA:        return sys::virtual_key::COMMA;
    case SDLK_EQUALS:       return sys::virtual_key::PLUS;
    case SDLK_MINUS:        return sys::virtual_key::MINUS;
    case SDLK_PERIOD:       return sys::virtual_key::PERIOD;
    case SDLK_SLASH:        return sys::virtual_key::FORWARD_SLASH;
    case SDLK_0:            return sys::virtual_key::NUM_0;
    case SDLK_1:            return sys::virtual_key::NUM_1;
    case SDLK_2:            return sys::virtual_key::NUM_2;
    case SDLK_3:            return sys::virtual_key::NUM_3;
    case SDLK_4:            return sys::virtual_key::NUM_4;
    case SDLK_5:            return sys::virtual_key::NUM_5;
    case SDLK_6:            return sys::virtual_key::NUM_6;
    case SDLK_7:            return sys::virtual_key::NUM_7;
    case SDLK_8:            return sys::virtual_key::NUM_8;
    case SDLK_9:            return sys::virtual_key::NUM_9;
    case SDLK_SEMICOLON:    return sys::virtual_key::SEMICOLON;
    case SDLK_a:            return sys::virtual_key::A;
    case SDLK_b:            return sys::virtual_key::B;
    case SDLK_c:            return sys::virtual_key::C;
    case SDLK_d:            return sys::virtual_key::D;
    case SDLK_e:            return sys::virtual_key::E;
    case SDLK_f:            return sys::virtual_key::F;
    case SDLK_g:            return sys::virtual_key::G;
    case SDLK_h:            return sys::virtual_key::H;
    case SDLK_i:            return sys::virtual_key::I;
    case SDLK_j:            return sys::virtual_key::J;
    case SDLK_k:            return sys::virtual_key::K;
    case SDLK_l:            return sys::virtual_key::L;
    case SDLK_m:            return sys::virtual_key::M;
    case SDLK_n:            return sys::virtual_key::N;
    case SDLK_o:            return sys::virtual_key::O;
    case SDLK_p:            return sys::virtual_key::P;
    case SDLK_q:            return sys::virtual_key::Q;
    case SDLK_r:            return sys::virtual_key::R;
    case SDLK_s:            return sys::virtual_key::S;
    case SDLK_t:            return sys::virtual_key::T;
    case SDLK_u:            return sys::virtual_key::U;
    case SDLK_v:            return sys::virtual_key::V;
    case SDLK_w:            return sys::virtual_key::W;
    case SDLK_x:            return sys::virtual_key::X;
    case SDLK_y:            return sys::virtual_key::Y;
    case SDLK_z:            return sys::virtual_key::Z;
    case SDLK_LEFTBRACKET:  return sys::virtual_key::OPEN_BRACKET;
    case SDLK_BACKSLASH:    return sys::virtual_key::BACK_SLASH;
    case SDLK_RIGHTBRACKET: return sys::virtual_key::CLOSED_BRACKET;
    case SDLK_BACKQUOTE:    return sys::virtual_key::TILDA;
    case SDLK_ESCAPE:       return sys::virtual_key::ESCAPE;
    case SDLK_AC_BACK:      return sys::virtual_key::ESCAPE;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:     return sys::virtual_key::RETURN;
    case SDLK_TAB:          return sys::virtual_key::TAB;
    case SDLK_BACKSPACE:    return sys::virtual_key::BACK;
    case SDLK_INSERT:       return sys::virtual_key::INSERT;
    case SDLK_DELETE:       return sys::virtual_key::DELETE_KEY;
    case SDLK_RIGHT:        return sys::virtual_key::RIGHT;
    case SDLK_LEFT:         return sys::virtual_key::LEFT;
    case SDLK_DOWN:         return sys::virtual_key::DOWN;
    case SDLK_UP:           return sys::virtual_key::UP;
    case SDLK_PAGEUP:       return sys::virtual_key::PRIOR;
    case SDLK_PAGEDOWN:     return sys::virtual_key::NEXT;
    case SDLK_HOME:         return sys::virtual_key::HOME;
    case SDLK_END:          return sys::virtual_key::END;
    case SDLK_F1:           return sys::virtual_key::F1;
    case SDLK_F2:           return sys::virtual_key::F2;
    case SDLK_F3:           return sys::virtual_key::F3;
    case SDLK_F4:           return sys::virtual_key::F4;
    case SDLK_F5:           return sys::virtual_key::F5;
    case SDLK_F6:           return sys::virtual_key::F6;
    case SDLK_F7:           return sys::virtual_key::F7;
    case SDLK_F8:           return sys::virtual_key::F8;
    case SDLK_F9:           return sys::virtual_key::F9;
    case SDLK_F10:          return sys::virtual_key::F10;
    case SDLK_F11:          return sys::virtual_key::F11;
    case SDLK_F12:          return sys::virtual_key::F12;
    case SDLK_LSHIFT:       return sys::virtual_key::LSHIFT;
    case SDLK_RSHIFT:       return sys::virtual_key::RSHIFT;
    case SDLK_LCTRL:        return sys::virtual_key::LCONTROL;
    case SDLK_RCTRL:        return sys::virtual_key::RCONTROL;
    case SDLK_LALT:         return sys::virtual_key::LMENU;
    case SDLK_RALT:         return sys::virtual_key::RMENU;
    default:                return sys::virtual_key::NONE;
    }
}

sys::key_modifiers sdl_modifiers_to_alice(Uint16 mod) noexcept {
    uint32_t val =
        uint32_t((mod & KMOD_CTRL)  ? sys::key_modifiers::modifiers_ctrl  : sys::key_modifiers::modifiers_none) |
        uint32_t((mod & KMOD_ALT)   ? sys::key_modifiers::modifiers_alt   : sys::key_modifiers::modifiers_none) |
        uint32_t((mod & KMOD_SHIFT) ? sys::key_modifiers::modifiers_shift : sys::key_modifiers::modifiers_none);
    return sys::key_modifiers(val);
}

bool is_point_over_ui(sys::state& state, int32_t x, int32_t y) {
    ui::element_base* root_elm = state.current_scene.get_root(state);
    if (!root_elm) return false;
    float ui_scale = (state.user_settings.ui_scale > 0.05f) ? state.user_settings.ui_scale : 1.0f;
    auto probe = root_elm->impl_probe_mouse(
        state,
        static_cast<int32_t>(x / ui_scale),
        static_cast<int32_t>(y / ui_scale),
        ui::mouse_probe_type::click);
    if (!probe.under_mouse) return false;

    // On-map unit counters belong to map space for camera panning & pinch zoom
    ui::element_base* b = probe.under_mouse;
    while (b) {
        if (b == state.ui_state.units_root.get() ||
            b == state.ui_state.unit_details_box.get() ||
            b == state.ui_state.unit_counter_box.get()) {
            return false;
        }
        b = b->parent;
    }
    return true;
}

void finish_active_drags(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mod) {
    if (g_map_pan_active) {
        state.map_state.on_mbuttom_up(x, y, mod);
        g_map_pan_active = false;
    }
    if (g_ui_drag_active) {
        state.on_lbutton_up(x, y, mod);
        if (state.win_ptr) state.win_ptr->left_mouse_down = false;
        g_ui_drag_active = false;
    }
}

void dispatch_touch_command(sys::state& state, const vic2_mobile::input::TouchCommand& cmd, float dpi_scale) {
    using vic2_mobile::input::TouchActionType;
    if (cmd.type == TouchActionType::None) return;

    auto& touch_ctrl = vic2_mobile::input::global_touch_controller();
    const sys::key_modifiers mod = sdl_modifiers_to_alice(SDL_GetModState());

    switch (cmd.type) {
    case TouchActionType::PrimaryTap: {
        state.mouse_x_position = cmd.x;
        state.mouse_y_position = cmd.y;
        state.on_mouse_move(cmd.x, cmd.y, mod);

        if (touch_ctrl.inspector_mode()) {
            // [?] Inspector Mode: pin Tooltip at (x, y) without firing button execution
            break;
        }
        state.on_lbutton_down(cmd.x, cmd.y, mod);
        if (state.win_ptr) state.win_ptr->left_mouse_down = true;
        state.on_lbutton_up(cmd.x, cmd.y, mod);
        if (state.win_ptr) state.win_ptr->left_mouse_down = false;
        break;
    }

    case TouchActionType::LongPressMarchOrTip: {
        state.mouse_x_position = cmd.x;
        state.mouse_y_position = cmd.y;
        state.on_mouse_move(cmd.x, cmd.y, mod);

        // On map: 0.30s hold issues Army/Navy march order (or diplomacy context);
        // On UI: 0.25s hold pins the detailed modifier/price Tooltip via on_mouse_move.
        if (!is_point_over_ui(state, cmd.x, cmd.y)) {
            state.on_rbutton_down(cmd.x, cmd.y, mod);
            state.on_rbutton_up(cmd.x, cmd.y, mod);
        }
        break;
    }

    case TouchActionType::PanMove: {
        state.mouse_x_position = cmd.x;
        state.mouse_y_position = cmd.y;

        if (touch_ctrl.box_select_mode()) {
            if (!g_ui_drag_active) {
                int32_t sx = cmd.x - static_cast<int32_t>(cmd.delta_x);
                int32_t sy = cmd.y - static_cast<int32_t>(cmd.delta_y);
                state.on_lbutton_down(sx, sy, mod);
                if (state.win_ptr) state.win_ptr->left_mouse_down = true;
                g_ui_drag_active = true;
            }
            state.on_mouse_move(cmd.x, cmd.y, mod);
            state.on_mouse_drag(cmd.x, cmd.y, mod);
        } else if (!g_map_pan_active && is_point_over_ui(state, cmd.x, cmd.y)) {
            // Smooth vertical touch scrolling inside UI listboxes & ledger tables
            float wheel_steps = cmd.delta_y / std::max(16.0f, 28.0f * dpi_scale);
            if (std::fabs(wheel_steps) > 0.01f) {
                sys::on_mouse_wheel(state, cmd.x, cmd.y, mod, wheel_steps);
            }
            state.on_mouse_move(cmd.x, cmd.y, mod);
        } else {
            // Map camera panning via Alice's projection-aware middle-mouse drag
            if (!g_map_pan_active) {
                int32_t sx = cmd.x - static_cast<int32_t>(cmd.delta_x);
                int32_t sy = cmd.y - static_cast<int32_t>(cmd.delta_y);
                state.map_state.on_mbuttom_down(sx, sy, state.x_size, state.y_size, mod);
                g_map_pan_active = true;
            }
            state.map_state.on_mouse_move(cmd.x, cmd.y, state.x_size, state.y_size, mod);
        }
        break;
    }

    case TouchActionType::PinchZoom: {
        finish_active_drags(state, cmd.x, cmd.y, mod);
        state.mouse_x_position = cmd.x;
        state.mouse_y_position = cmd.y;
        sys::on_mouse_wheel(state, cmd.x, cmd.y, mod, cmd.zoom_delta * 2.5f);
        break;
    }

    case TouchActionType::ToggleDebugConsole: {
        finish_active_drags(state, cmd.x, cmd.y, mod);
        ui::console_window::show_toggle(state);
        break;
    }

    default:
        break;
    }
}

void refresh_drawable_viewport(SDL_Window* sdl_win, sys::state& state) {
    int width = 0, height = 0;
    SDL_GL_GetDrawableSize(sdl_win, &width, &height);
    if (width <= 0 || height <= 0) {
        SDL_GetWindowSize(sdl_win, &width, &height);
    }
    if (width <= 0)  width = 1920;
    if (height <= 0) height = 1080;

    float density = 2.5f;
    vic2_mobile::input::SafeAreaInsets insets{};
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        density = vic2_mobile::runtime::config().display_density;
        insets  = vic2_mobile::runtime::config().insets;
    }
    vic2_mobile::input::global_touch_controller().set_viewport(width, height, density, insets);

    if (width != state.x_size || height != state.y_size) {
        state.x_size = width;
        state.y_size = height;
        state.on_resize(width, height, window_state::maximized);
    }
}

void save_background_snapshot_if_ingame(sys::state& state) {
    if (state.current_scene.id != game_scene::scene_id::pick_nation &&
        state.current_scene.id != game_scene::scene_id::end_screen &&
        state.local_player_nation) {
        VIC2_LOGI("App entering background: writing fast zstd suspend snapshot...");
        sys::write_save_file(state, sys::save_type::autosave, "mobile_suspend_snapshot", "mobile_suspend_snapshot");
        VIC2_LOGI("Background suspend snapshot saved.");
    }
}

void dispatch_utf8_text_input(sys::state& state, const char* utf8) {
    if (!utf8 || !state.ui_state.edit_target_internal) return;
    const auto* p = reinterpret_cast<const uint8_t*>(utf8);
    while (*p) {
        char32_t cp = 0;
        if ((*p & 0x80u) == 0) {
            cp = *p++;
        } else if ((*p & 0xE0u) == 0xC0u && p[1]) {
            cp = ((p[0] & 0x1Fu) << 6) | (p[1] & 0x3Fu);
            p += 2;
        } else if ((*p & 0xF0u) == 0xE0u && p[1] && p[2]) {
            cp = ((p[0] & 0x0Fu) << 12) | ((p[1] & 0x3Fu) << 6) | (p[2] & 0x3Fu);
            p += 3;
        } else if ((*p & 0xF8u) == 0xF0u && p[1] && p[2] && p[3]) {
            cp = ((p[0] & 0x07u) << 18) | ((p[1] & 0x3Fu) << 12) | ((p[2] & 0x3Fu) << 6) | (p[3] & 0x3Fu);
            p += 4;
        } else {
            ++p;
            continue;
        }
        state.on_text(cp);
    }
}

} // namespace

int32_t cursor_blink_ms() { return 1000; }
int32_t double_click_ms() { return 400; }

bool is_key_depressed(sys::state const& /*game_state*/, sys::virtual_key key) {
    int num_keys = 0;
    const Uint8* state = SDL_GetKeyboardState(&num_keys);
    if (!state) return false;
    for (int sc = 0; sc < num_keys; ++sc) {
        if (state[sc] && sdl_keycode_to_virtual_key(SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(sc))) == key) {
            return true;
        }
    }
    return false;
}

void get_window_size(sys::state const& game_state, int& width, int& height) {
    if (game_state.win_ptr && game_state.win_ptr->window) {
        auto* sdl_win = reinterpret_cast<SDL_Window*>(game_state.win_ptr->window);
        SDL_GL_GetDrawableSize(sdl_win, &width, &height);
        if (width > 0 && height > 0) return;
    }
    width = game_state.x_size > 0 ? game_state.x_size : 1920;
    height = game_state.y_size > 0 ? game_state.y_size : 1080;
}

bool is_in_fullscreen(sys::state const& game_state) {
    return !game_state.win_ptr || game_state.win_ptr->in_fullscreen;
}

void set_borderless_full_screen(sys::state& game_state, bool fullscreen) {
    if (game_state.win_ptr && game_state.win_ptr->window) {
        auto* sdl_win = reinterpret_cast<SDL_Window*>(game_state.win_ptr->window);
        SDL_SetWindowFullscreen(sdl_win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        game_state.win_ptr->in_fullscreen = fullscreen;
        refresh_drawable_viewport(sdl_win, game_state);
    }
}

void close_window(sys::state& /*game_state*/) {
    g_window_should_close.store(true, std::memory_order_release);
}

void create_window(sys::state& game_state, creation_parameters const& params) {
    g_window_should_close.store(false, std::memory_order_release);
    game_state.win_ptr = std::make_unique<window_data_impl>();
    game_state.win_ptr->creation_x_size = params.size_x;
    game_state.win_ptr->creation_y_size = params.size_y;
    game_state.win_ptr->in_fullscreen = true;

    // Prevent SDL from generating raw synthetic LClick mouse events on touch
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        emit_error_message(std::string("Failed to initialize SDL2: ") + SDL_GetError(), true);
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    uint32_t win_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE |
                         SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FULLSCREEN;
    SDL_Window* sdl_win = SDL_CreateWindow(
        "Victoria II Mobile",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        params.size_x, params.size_y,
        win_flags);
    if (!sdl_win) {
        emit_error_message(std::string("Failed to create SDL2 window: ") + SDL_GetError(), true);
        return;
    }

    game_state.win_ptr->window = reinterpret_cast<GLFWwindow*>(sdl_win);

    // Auto-scale UI on high-DPI 20:9 mobile screens when using default 1.0x settings
    float density = 2.5f;
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        density = vic2_mobile::runtime::config().display_density;
    }
    int init_w = 0, init_h = 0;
    SDL_GL_GetDrawableSize(sdl_win, &init_w, &init_h);
    if (init_h > 0 && std::fabs(game_state.user_settings.ui_scale - 1.0f) < 0.01f) {
        // Calibrate UI scale so 40dp topbar and 768p V2 dialogs fit comfortably in landscape
        float target_scale = std::clamp(static_cast<float>(init_h) / 768.0f, 0.85f, 1.65f);
        game_state.user_settings.ui_scale = target_scale;
    }

    ogl::initialize_opengl(game_state);

    sound::initialize_sound_system(game_state);
    sound::start_music(game_state, game_state.user_settings.master_volume * game_state.user_settings.music_volume);

    refresh_drawable_viewport(sdl_win, game_state);

    change_cursor(game_state, cursor_type::busy);
    game_state.on_create();
    change_cursor(game_state, cursor_type::normal_cancel_busy);

    vic2_mobile::runtime::set_status(3, "In-game OpenGL ES 3.2 loop running");

    bool soft_keyboard_open = false;
    bool app_in_background = false;
    auto& touch_ctrl = vic2_mobile::input::global_touch_controller();

    while (!g_window_should_close.load(std::memory_order_acquire)) {
        uint64_t now_ms = SDL_GetTicks64();

        // Dynamic 15 FPS idle throttle when simulation is paused and screen untouched > 1.5s
        bool sim_paused = (game_state.actual_game_speed.load(std::memory_order_relaxed) <= 0) ||
                          game_state.ui_pause.load(std::memory_order_relaxed) ||
                          app_in_background;
        uint32_t idle_delay_ms = app_in_background ? 200u : touch_ctrl.target_frame_delay_ms(now_ms, sim_paused);
        if (idle_delay_ms > 0) {
            SDL_WaitEventTimeout(nullptr, static_cast<int>(idle_delay_ms));
            now_ms = SDL_GetTicks64();
        }

        {
            std::shared_lock lock(game_state.game_state_resetting_lock);
            game_state.game_state_resetting_cv.wait(lock, [&] { return !game_state.yield_game_state_resetting_lock; });

            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                now_ms = SDL_GetTicks64();
                switch (ev.type) {
                case SDL_QUIT:
                case SDL_APP_TERMINATING:
                    save_background_snapshot_if_ingame(game_state);
                    g_window_should_close.store(true, std::memory_order_release);
                    break;

                case SDL_APP_WILLENTERBACKGROUND:
                    app_in_background = true;
                    sound::pause_all(game_state);
                    save_background_snapshot_if_ingame(game_state);
                    break;

                case SDL_APP_DIDENTERFOREGROUND:
                    app_in_background = false;
                    sound::resume_all(game_state);
                    refresh_drawable_viewport(sdl_win, game_state);
                    break;

                case SDL_WINDOWEVENT:
                    if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                        ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                        refresh_drawable_viewport(sdl_win, game_state);
                    }
                    break;

                case SDL_FINGERDOWN: {
                    auto cmd = touch_ctrl.on_finger_down(ev.tfinger.fingerId, ev.tfinger.x, ev.tfinger.y, now_ms);
                    dispatch_touch_command(game_state, cmd, density);
                    break;
                }

                case SDL_FINGERMOTION: {
                    auto cmd = touch_ctrl.on_finger_motion(ev.tfinger.fingerId, ev.tfinger.x, ev.tfinger.y, now_ms);
                    dispatch_touch_command(game_state, cmd, density);
                    break;
                }

                case SDL_FINGERUP: {
                    auto cmd = touch_ctrl.on_finger_up(ev.tfinger.fingerId, ev.tfinger.x, ev.tfinger.y, now_ms);
                    int32_t px = static_cast<int32_t>(std::lround(ev.tfinger.x * game_state.x_size));
                    int32_t py = static_cast<int32_t>(std::lround(ev.tfinger.y * game_state.y_size));
                    finish_active_drags(game_state, px, py, sdl_modifiers_to_alice(SDL_GetModState()));
                    dispatch_touch_command(game_state, cmd, density);
                    break;
                }

                // Hardware mouse fallback (Bluetooth mouse / Android Emulator pointer)
                case SDL_MOUSEMOTION:
                    if (ev.motion.which != SDL_TOUCH_MOUSEID) {
                        auto mod = sdl_modifiers_to_alice(SDL_GetModState());
                        game_state.on_mouse_move(ev.motion.x, ev.motion.y, mod);
                        if (ev.motion.state & SDL_BUTTON_LMASK) {
                            game_state.on_mouse_drag(ev.motion.x, ev.motion.y, mod);
                        }
                        game_state.mouse_x_position = ev.motion.x;
                        game_state.mouse_y_position = ev.motion.y;
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (ev.button.which != SDL_TOUCH_MOUSEID) {
                        auto mod = sdl_modifiers_to_alice(SDL_GetModState());
                        if (ev.button.button == SDL_BUTTON_LEFT) {
                            game_state.on_lbutton_down(ev.button.x, ev.button.y, mod);
                            game_state.win_ptr->left_mouse_down = true;
                        } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                            game_state.on_rbutton_down(ev.button.x, ev.button.y, mod);
                        } else if (ev.button.button == SDL_BUTTON_MIDDLE) {
                            game_state.on_mbutton_down(ev.button.x, ev.button.y, mod);
                        }
                        game_state.mouse_x_position = ev.button.x;
                        game_state.mouse_y_position = ev.button.y;
                    }
                    break;

                case SDL_MOUSEBUTTONUP:
                    if (ev.button.which != SDL_TOUCH_MOUSEID) {
                        auto mod = sdl_modifiers_to_alice(SDL_GetModState());
                        if (ev.button.button == SDL_BUTTON_LEFT) {
                            game_state.on_lbutton_up(ev.button.x, ev.button.y, mod);
                            game_state.win_ptr->left_mouse_down = false;
                        } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                            game_state.on_rbutton_up(ev.button.x, ev.button.y, mod);
                        } else if (ev.button.button == SDL_BUTTON_MIDDLE) {
                            game_state.on_mbutton_up(ev.button.x, ev.button.y, mod);
                        }
                        game_state.mouse_x_position = ev.button.x;
                        game_state.mouse_y_position = ev.button.y;
                    }
                    break;

                case SDL_MOUSEWHEEL:
                    if (ev.wheel.which != SDL_TOUCH_MOUSEID) {
                        sys::on_mouse_wheel(
                            game_state,
                            game_state.mouse_x_position,
                            game_state.mouse_y_position,
                            sdl_modifiers_to_alice(SDL_GetModState()),
                            ev.wheel.preciseY);
                    }
                    break;

                case SDL_KEYDOWN: {
                    auto vk = sdl_keycode_to_virtual_key(ev.key.keysym.sym);
                    if (vk != sys::virtual_key::NONE) {
                        game_state.on_key_down(vk, sdl_modifiers_to_alice(ev.key.keysym.mod));
                    }
                    break;
                }

                case SDL_KEYUP: {
                    auto vk = sdl_keycode_to_virtual_key(ev.key.keysym.sym);
                    if (vk != sys::virtual_key::NONE) {
                        game_state.on_key_up(vk, sdl_modifiers_to_alice(ev.key.keysym.mod));
                    }
                    break;
                }

                case SDL_TEXTINPUT:
                    dispatch_utf8_text_input(game_state, ev.text.text);
                    break;

                default:
                    break;
                }
            }

            // Poll stationary hold timer (0.25s UI Tooltip / 0.30s Map Army March)
            bool hold_over_ui = is_point_over_ui(game_state, game_state.mouse_x_position, game_state.mouse_y_position);
            auto hold_cmd = touch_ctrl.poll_hold(SDL_GetTicks64(), hold_over_ui);
            dispatch_touch_command(game_state, hold_cmd, density);

            // Sync Android soft keyboard with active edit box (console, search, save name)
            bool want_keyboard = (game_state.ui_state.edit_target_internal != nullptr);
            if (want_keyboard != soft_keyboard_open) {
                soft_keyboard_open = want_keyboard;
                if (soft_keyboard_open) {
                    SDL_StartTextInput();
                } else {
                    SDL_StopTextInput();
                }
            }

            if (!app_in_background) {
                game_state.render();
                SDL_GL_SwapWindow(sdl_win);
            }
        }

        if (!app_in_background) {
            sound::update_music_track(game_state);
        }
    }

    ogl::shutdown_opengl(game_state);
    SDL_DestroyWindow(sdl_win);
    game_state.win_ptr->window = nullptr;
    SDL_Quit();
}

void change_cursor(sys::state& /*state*/, cursor_type /*type*/) {
    // Touch-first mobile interface does not render hardware desktop cursors
}

void emit_error_message(std::string const& content, bool fatal) {
    if (fatal) {
        VIC2_LOGE("FATAL: %s", content.c_str());
        vic2_mobile::runtime::set_status(-1, content);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Victoria II Mobile Error", content.c_str(), nullptr);
        std::exit(EXIT_FAILURE);
    } else {
        VIC2_LOGW("%s", content.c_str());
    }
}

win32_text_services::win32_text_services() = default;
win32_text_services::~win32_text_services() = default;
void win32_text_services::start_text_services() {}
void win32_text_services::end_text_services() {}
void win32_text_services::on_text_change(text_services_object*, uint32_t, uint32_t, uint32_t) {}
void win32_text_services::on_selection_change(text_services_object*) {}
bool win32_text_services::send_mouse_event_to_tso(text_services_object*, int32_t, int32_t, uint32_t) { return false; }
void win32_text_services::set_focus(sys::state&, text_services_object*) {}
void win32_text_services::suspend_keystroke_handling() {}
void win32_text_services::resume_keystroke_handling() {}
text_services_object* win32_text_services::create_text_service_object(sys::state&, ui::element_base&) { return nullptr; }
void release_text_services_object(text_services_object*) {}

} // namespace window
