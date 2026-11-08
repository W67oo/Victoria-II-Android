#include "mobile_dock_bar.hpp"
#include "system_state.hpp"
#include "nine_slice_renderer.hpp"
#include "runtime_config_android.hpp"
#include <algorithm>

namespace ui {

void mobile_dock_bar_window::on_create(sys::state& state) noexcept {
    container_base::on_create(state);
    impl_on_update(state);
}

void mobile_dock_bar_window::impl_on_update(sys::state& state) noexcept {
    vic2_mobile::input::SafeAreaInsets insets{};
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        insets = vic2_mobile::runtime::config().insets;
    }

    base_data.position.x = int16_t(insets.left);
    base_data.position.y = 46;
    base_data.size.x = 52;
    base_data.size.y = int16_t(std::max(100, state.y_size - 46 - insets.bottom));

    container_base::impl_on_update(state);
}

DockTab mobile_dock_bar_window::probe_tab(int32_t py) const noexcept {
    int32_t rel_y = py - base_data.position.y;
    if (rel_y < 0) return DockTab::None;

    const int32_t item_h = 48;
    int32_t index = rel_y / item_h;
    if (index >= 0 && index < static_cast<int32_t>(DockTab::Count)) {
        return static_cast<DockTab>(index);
    }
    return DockTab::None;
}

element_base* mobile_dock_bar_window::get_window_for_tab(sys::state& state, DockTab tab) noexcept {
    switch (tab) {
    case DockTab::Production: return state.ui_state.production_subwindow;
    case DockTab::Budget:     return state.ui_state.topbar_subwindow;
    case DockTab::Technology: return state.ui_state.technology_subwindow;
    case DockTab::Politics:   return state.ui_state.politics_subwindow;
    case DockTab::Population: return state.ui_state.population_subwindow;
    case DockTab::Trade:      return state.ui_state.trade_subwindow;
    case DockTab::Diplomacy:  return state.ui_state.request_window;
    case DockTab::Military:   return state.ui_state.military_subwindow;
    default: return nullptr;
    }
}

mouse_probe mobile_dock_bar_window::impl_probe_mouse(sys::state& /*state*/, int32_t x, int32_t y, mouse_probe_type /*type*/) noexcept {
    if (x >= base_data.position.x && x < base_data.position.x + base_data.size.x &&
        y >= base_data.position.y && y < base_data.position.y + base_data.size.y) {
        hovered_tab_ = probe_tab(y);
        return mouse_probe{ this, xy_pair{ int16_t(x), int16_t(y) } };
    }
    return mouse_probe{};
}

message_result mobile_dock_bar_window::impl_on_lbutton_down(sys::state& /*state*/, int32_t /*x*/, int32_t /*y*/, sys::key_modifiers /*mods*/) noexcept {
    return message_result::consumed;
}

message_result mobile_dock_bar_window::impl_on_lbutton_up(sys::state& state, int32_t /*x*/, int32_t y, sys::key_modifiers /*mods*/, bool under_mouse) noexcept {
    if (!under_mouse) return message_result::unseen;

    DockTab tab = probe_tab(y);
    if (tab != DockTab::None) {
        toggle_tab(state, tab);
    }
    return message_result::consumed;
}

void mobile_dock_bar_window::toggle_tab(sys::state& state, DockTab tab) noexcept {
    // If clicking already open tab, close it
    if (active_tab_ == tab) {
        close_active_tab(state);
        return;
    }

    // Close previously active tab
    close_active_tab(state);

    element_base* win = get_window_for_tab(state, tab);
    if (win) {
        vic2_mobile::input::SafeAreaInsets insets{};
        {
            std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
            insets = vic2_mobile::runtime::config().insets;
        }

        // Center modal window taking ~85% viewport, leaving dock visible on left
        int32_t target_x = insets.left + base_data.size.x + 10;
        int32_t target_y = 48;
        int32_t target_w = std::max(200, state.x_size - target_x - insets.right - 10);
        int32_t target_h = std::max(200, state.y_size - target_y - insets.bottom - 10);

        win->base_data.position.x = int16_t(target_x);
        win->base_data.position.y = int16_t(target_y);
        win->base_data.size.x = int16_t(target_w);
        win->base_data.size.y = int16_t(target_h);

        win->set_visible(state, true);
        if (state.ui_state.root) {
            state.ui_state.root->move_child_to_front(win);
        }
        active_tab_ = tab;
    }
}

void mobile_dock_bar_window::close_active_tab(sys::state& state) noexcept {
    if (active_tab_ != DockTab::None) {
        element_base* win = get_window_for_tab(state, active_tab_);
        if (win) {
            win->set_visible(state, false);
        }
        active_tab_ = DockTab::None;
    }
}

void mobile_dock_bar_window::render(sys::state& state, int32_t x, int32_t y) noexcept {
    // 1. Render 9-slice vertical dock rail frame
    ui::render_nine_slice(
        state,
        ui::slice_presets::drawer_panel,
        float(x + base_data.position.x),
        float(y + base_data.position.y),
        float(base_data.size.x),
        float(base_data.size.y),
        state.ui_state.bg_gfx_id
    );

    // 2. Render children / icon badges
    container_base::render(state, x, y);
}

void mobile_dock_bar_window::update_tooltip(sys::state& state, int32_t /*x*/, int32_t /*y*/, text::columnar_layout& contents) noexcept {
    auto box = text::open_layout_box(contents, 0);
    switch (hovered_tab_) {
    case DockTab::Production: text::add_to_layout_box(state, contents, box, std::string_view("Production & Factories (生产)")); break;
    case DockTab::Budget:     text::add_to_layout_box(state, contents, box, std::string_view("Budget & Central Bank (财政与央行)")); break;
    case DockTab::Technology: text::add_to_layout_box(state, contents, box, std::string_view("Technology & Research (科技)")); break;
    case DockTab::Politics:   text::add_to_layout_box(state, contents, box, std::string_view("Politics & Reforms (政治与改革)")); break;
    case DockTab::Population: text::add_to_layout_box(state, contents, box, std::string_view("Demographics & POPs (人口与阶层)")); break;
    case DockTab::Trade:      text::add_to_layout_box(state, contents, box, std::string_view("Trade & Common Market (市场与贸易)")); break;
    case DockTab::Diplomacy:  text::add_to_layout_box(state, contents, box, std::string_view("Diplomacy & Crises (外交与危机)")); break;
    case DockTab::Military:   text::add_to_layout_box(state, contents, box, std::string_view("Military & Armed Forces (陆海军力)")); break;
    default: break;
    }
    text::close_layout_box(contents, box);
}

std::unique_ptr<mobile_dock_bar_window> create_mobile_dock_bar(sys::state& state) {
    auto dock = std::make_unique<mobile_dock_bar_window>();
    dock->on_create(state);
    return dock;
}

} // namespace ui
