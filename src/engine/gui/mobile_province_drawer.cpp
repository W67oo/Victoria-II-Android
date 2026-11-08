#include "mobile_province_drawer.hpp"
#include "system_state.hpp"
#include "nine_slice_renderer.hpp"
#include "runtime_config_android.hpp"
#include "demographics.hpp"
#include <fmt/format.h>
#include <algorithm>

namespace ui {

void mobile_province_drawer_window::on_create(sys::state& state) noexcept {
    container_base::on_create(state);
    set_visible(state, false);
    impl_on_update(state);
}

void mobile_province_drawer_window::impl_on_update(sys::state& state) noexcept {
    vic2_mobile::input::SafeAreaInsets insets{};
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        insets = vic2_mobile::runtime::config().insets;
    }

    // Right-aligned half-screen drawer (approx 46% of width, min 320, max 600)
    int32_t drawer_w = std::clamp(static_cast<int32_t>(state.x_size * 0.46f), 320, 600);
    int32_t drawer_h = std::max(200, state.y_size - 44 - insets.bottom);

    base_data.size = xy_pair{ int16_t(drawer_w), int16_t(drawer_h) };
    base_data.position.x = int16_t(state.x_size - insets.right - drawer_w);
    base_data.position.y = 44;

    container_base::impl_on_update(state);
}

void mobile_province_drawer_window::open_province(sys::state& state, dcon::province_id prov) noexcept {
    if (!state.world.province_is_valid(prov)) return;

    active_province_ = prov;
    is_open_ = true;
    set_visible(state, true);
    impl_on_update(state);

    if (state.ui_state.root) {
        state.ui_state.root->move_child_to_front(this);
    }
}

void mobile_province_drawer_window::close_drawer(sys::state& state) noexcept {
    is_open_ = false;
    set_visible(state, false);
}

int32_t mobile_province_drawer_window::probe_drawer_item(int32_t px, int32_t py) const noexcept {
    if (!is_open_) return -1;

    int32_t rel_x = px - base_data.position.x;
    int32_t rel_y = py - base_data.position.y;

    // Close button [X] at top-right
    if (rel_x >= base_data.size.x - 44 && rel_y <= 44) return 0;

    // 3 Tab selector bar at rel_y in [44, 84]
    if (rel_y >= 44 && rel_y <= 84) {
        int32_t tab_w = base_data.size.x / 3;
        int32_t tab_idx = rel_x / tab_w;
        if (tab_idx >= 0 && tab_idx < 3) {
            return 1 + tab_idx; // 1: Overview, 2: RGO, 3: Factories
        }
    }

    return -1;
}

mouse_probe mobile_province_drawer_window::impl_probe_mouse(sys::state& /*state*/, int32_t x, int32_t y, mouse_probe_type /*type*/) noexcept {
    if (!is_open_) return mouse_probe{};

    if (x >= base_data.position.x && x < base_data.position.x + base_data.size.x &&
        y >= base_data.position.y && y < base_data.position.y + base_data.size.y) {
        hovered_item_ = probe_drawer_item(x, y);
        return mouse_probe{ this, xy_pair{ int16_t(x), int16_t(y) } };
    }
    return mouse_probe{};
}

message_result mobile_province_drawer_window::impl_on_lbutton_down(sys::state& /*state*/, int32_t /*x*/, int32_t /*y*/, sys::key_modifiers /*mods*/) noexcept {
    return message_result::consumed;
}

message_result mobile_province_drawer_window::impl_on_lbutton_up(sys::state& state, int32_t x, int32_t y, sys::key_modifiers /*mods*/, bool under_mouse) noexcept {
    if (!under_mouse || !is_open_) return message_result::unseen;

    int32_t item = probe_drawer_item(x, y);
    if (item == 0) {
        // Close button
        close_drawer(state);
        return message_result::consumed;
    }

    if (item >= 1 && item <= 3) {
        // Tab switch
        active_tab_ = static_cast<ProvinceDrawerTab>(item - 1);
        return message_result::consumed;
    }

    return message_result::consumed;
}

void mobile_province_drawer_window::render(sys::state& state, int32_t x, int32_t y) noexcept {
    if (!is_open_ || !state.world.province_is_valid(active_province_)) return;

    // 1. Render 9-slice right drawer frame
    ui::render_nine_slice(
        state,
        ui::slice_presets::drawer_panel,
        float(x + base_data.position.x),
        float(y + base_data.position.y),
        float(base_data.size.x),
        float(base_data.size.y),
        state.ui_state.bg_gfx_id
    );

    // 2. Render children & content
    container_base::render(state, x, y);
}

void mobile_province_drawer_window::update_tooltip(sys::state& state, int32_t /*x*/, int32_t /*y*/, text::columnar_layout& contents) noexcept {
    if (!is_open_ || !state.world.province_is_valid(active_province_)) return;

    auto box = text::open_layout_box(contents, 0);

    if (hovered_item_ == 0) {
        text::add_to_layout_box(state, contents, box, std::string_view("Close Province Drawer (关闭抽屉)"));
    } else if (hovered_item_ == 1) {
        text::add_to_layout_box(state, contents, box, std::string_view("Overview & Military Tab (概览与军事)"));
    } else if (hovered_item_ == 2) {
        text::add_to_layout_box(state, contents, box, std::string_view("RGO & Local Market Tab (物产与物价)"));
    } else if (hovered_item_ == 3) {
        text::add_to_layout_box(state, contents, box, std::string_view("Factories & Construction Tab (工业与建设)"));
    } else {
        float pop_total = state.world.province_get_demographics(active_province_, demographics::total);
        auto rgo = state.world.province_get_rgo(active_province_);
        text::add_to_layout_box(state, contents, box, fmt::format("Province Total POP: {:.0f} | RGO: {}", pop_total, text::produce_simple_string(state, state.world.commodity_get_name(rgo))));
    }

    text::close_layout_box(contents, box);
}

std::unique_ptr<mobile_province_drawer_window> create_mobile_province_drawer(sys::state& state) {
    auto drawer = std::make_unique<mobile_province_drawer_window>();
    drawer->on_create(state);
    return drawer;
}

} // namespace ui
