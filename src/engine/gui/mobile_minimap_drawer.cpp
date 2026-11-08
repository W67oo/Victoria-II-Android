#include "mobile_minimap_drawer.hpp"
#include "system_state.hpp"
#include "map_modes.hpp"
#include "nine_slice_renderer.hpp"
#include "runtime_config_android.hpp"
#include <algorithm>

namespace ui {

void mobile_minimap_drawer_window::on_create(sys::state& state) noexcept {
    container_base::on_create(state);
    impl_on_update(state);
}

void mobile_minimap_drawer_window::impl_on_update(sys::state& state) noexcept {
    vic2_mobile::input::SafeAreaInsets insets{};
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        insets = vic2_mobile::runtime::config().insets;
    }

    if (!is_expanded_) {
        // Collapsed floating button
        base_data.size = xy_pair{ 48, 48 };
        base_data.position.x = int16_t(state.x_size - insets.right - 56);
        base_data.position.y = int16_t(state.y_size - insets.bottom - 56);
    } else {
        // Expanded minimap tray
        base_data.size = xy_pair{ 260, 200 };
        base_data.position.x = int16_t(state.x_size - insets.right - 268);
        base_data.position.y = int16_t(state.y_size - insets.bottom - 208);
    }

    container_base::impl_on_update(state);
}

int32_t mobile_minimap_drawer_window::probe_sub_item(int32_t px, int32_t py) const noexcept {
    if (!is_expanded_) {
        return 0; // The floating compass button itself
    }

    int32_t rel_x = px - base_data.position.x;
    int32_t rel_y = py - base_data.position.y;

    // Compass toggle button in top-right of tray
    if (rel_x >= base_data.size.x - 36 && rel_y <= 36) return 0;

    // Mapmode buttons in bottom grid: 2 rows of 4 items
    if (rel_y >= 120 && rel_y <= 190) {
        int32_t row = (rel_y - 120) / 35;
        int32_t col = rel_x / 62;
        int32_t mode_idx = row * 4 + col;
        if (mode_idx >= 0 && mode_idx < 8) {
            return 1 + mode_idx; // 1 ~ 8: Map modes
        }
    }

    // Top action buttons: Goto (9) and Ledger (10)
    if (rel_y >= 8 && rel_y <= 40) {
        if (rel_x >= 8 && rel_x <= 60) return 9;   // Search / Goto
        if (rel_x >= 68 && rel_x <= 120) return 10; // Ledger
    }

    return -1;
}

mouse_probe mobile_minimap_drawer_window::impl_probe_mouse(sys::state& /*state*/, int32_t x, int32_t y, mouse_probe_type /*type*/) noexcept {
    if (x >= base_data.position.x && x < base_data.position.x + base_data.size.x &&
        y >= base_data.position.y && y < base_data.position.y + base_data.size.y) {
        hovered_item_ = probe_sub_item(x, y);
        return mouse_probe{ this, xy_pair{ int16_t(x), int16_t(y) } };
    }
    return mouse_probe{};
}

message_result mobile_minimap_drawer_window::impl_on_lbutton_down(sys::state& /*state*/, int32_t /*x*/, int32_t /*y*/, sys::key_modifiers /*mods*/) noexcept {
    return message_result::consumed;
}

message_result mobile_minimap_drawer_window::impl_on_lbutton_up(sys::state& state, int32_t x, int32_t y, sys::key_modifiers /*mods*/, bool under_mouse) noexcept {
    if (!under_mouse) return message_result::unseen;

    int32_t item = probe_sub_item(x, y);
    if (item == 0) {
        // Toggle expanded / collapsed state
        is_expanded_ = !is_expanded_;
        impl_on_update(state);
        return message_result::consumed;
    }

    if (is_expanded_) {
        // Map mode selection:
        const map_mode::mode modes[8] = {
            map_mode::mode::terrain,
            map_mode::mode::political,
            map_mode::mode::diplomatic,
            map_mode::mode::crisis,
            map_mode::mode::rgo_output,
            map_mode::mode::infrastructure,
            map_mode::mode::population,
            map_mode::mode::sphere
        };
        if (item >= 1 && item <= 8) {
            map_mode::set_map_mode(state, modes[item - 1]);
            return message_result::consumed;
        }

        if (item == 9 && state.ui_state.search_window) {
            bool vis = state.ui_state.search_window->is_visible();
            state.ui_state.search_window->set_visible(state, !vis);
            if (!vis) state.ui_state.root->move_child_to_front(state.ui_state.search_window);
            return message_result::consumed;
        }

        if (item == 10 && state.ui_state.ledger_window) {
            bool vis = state.ui_state.ledger_window->is_visible();
            state.ui_state.ledger_window->set_visible(state, !vis);
            if (!vis) state.ui_state.root->move_child_to_front(state.ui_state.ledger_window);
            return message_result::consumed;
        }
    }

    return message_result::consumed;
}

void mobile_minimap_drawer_window::render(sys::state& state, int32_t x, int32_t y) noexcept {
    if (!is_expanded_) {
        // 1. Render collapsed round brass compass button
        ui::render_nine_slice(
            state,
            ui::slice_presets::brass_frame,
            float(x + base_data.position.x),
            float(y + base_data.position.y),
            float(base_data.size.x),
            float(base_data.size.y),
            state.ui_state.bg_gfx_id
        );
    } else {
        // 2. Render expanded tray frame
        ui::render_nine_slice(
            state,
            ui::slice_presets::parchment_panel,
            float(x + base_data.position.x),
            float(y + base_data.position.y),
            float(base_data.size.x),
            float(base_data.size.y),
            state.ui_state.bg_gfx_id
        );
    }

    container_base::render(state, x, y);
}

void mobile_minimap_drawer_window::update_tooltip(sys::state& state, int32_t /*x*/, int32_t /*y*/, text::columnar_layout& contents) noexcept {
    auto box = text::open_layout_box(contents, 0);
    if (!is_expanded_ || hovered_item_ == 0) {
        text::add_to_layout_box(state, contents, box, std::string_view("Minimap & Map Modes (Tap to Expand/Collapse)"));
    } else if (hovered_item_ >= 1 && hovered_item_ <= 8) {
        const char* names[8] = {
            "Terrain Mode (地形)",
            "Political Mode (政治)",
            "Diplomatic Mode (外交)",
            "Crisis Tension Mode (危机)",
            "RGO Goods Mode (资源物产)",
            "Infrastructure Mode (铁路基建)",
            "Population Density (人口密度)",
            "Sphere of Influence (势力范围)"
        };
        text::add_to_layout_box(state, contents, box, std::string_view(names[hovered_item_ - 1]));
    } else if (hovered_item_ == 9) {
        text::add_to_layout_box(state, contents, box, std::string_view("Search & Goto Province (找地)"));
    } else if (hovered_item_ == 10) {
        text::add_to_layout_box(state, contents, box, std::string_view("World Ledger (账本榜单)"));
    }
    text::close_layout_box(contents, box);
}

std::unique_ptr<mobile_minimap_drawer_window> create_mobile_minimap_drawer(sys::state& state) {
    auto drawer = std::make_unique<mobile_minimap_drawer_window>();
    drawer->on_create(state);
    return drawer;
}

} // namespace ui
