#pragma once
// =============================================================================
// mobile_dock_bar.hpp
// Vertical Left-Rail 8-Department Drawer Dock for 20:9 Mobile Screens
// Replaces crowded PC horizontal topbar tabs with a touch-friendly vertical rail:
//   1. Production (工厂 / 产业)
//   2. Budget & Central Bank (财政 / 央行)
//   3. Technology (科技 / 研发)
//   4. Politics (政治 / 改革)
//   5. Population (人口 / 阶层)
//   6. Trade & Market (贸易 / 市场)
//   7. Diplomacy (外交 / 危机)
//   8. Military (军事 / 陆海军)
// Features:
//   - Single-focus window management (toggling one closes previous)
//   - Centered 85% modal dialog placement with edge dismiss
// =============================================================================

#include "gui_element_types.hpp"
#include "system_state_forward.hpp"
#include <cstdint>
#include <memory>

namespace ui {

enum class DockTab : int8_t {
    None = -1,
    Production = 0,
    Budget = 1,
    Technology = 2,
    Politics = 3,
    Population = 4,
    Trade = 5,
    Diplomacy = 6,
    Military = 7,
    Count = 8
};

class mobile_dock_bar_window : public container_base {
public:
    mobile_dock_bar_window() = default;

    void on_create(sys::state& state) noexcept override;
    void impl_on_update(sys::state& state) noexcept override;
    void render(sys::state& state, int32_t x, int32_t y) noexcept override;

    mouse_probe impl_probe_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type type) noexcept override;
    message_result impl_on_lbutton_down(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods) noexcept override;
    message_result impl_on_lbutton_up(sys::state& state, int32_t x, int32_t y, sys::key_modifiers mods, bool under_mouse) noexcept override;

    tooltip_behavior has_tooltip(sys::state& state) noexcept override {
        return tooltip_behavior::variable_tooltip;
    }
    void update_tooltip(sys::state& state, int32_t x, int32_t y, text::columnar_layout& contents) noexcept override;

    void toggle_tab(sys::state& state, DockTab tab) noexcept;
    void close_active_tab(sys::state& state) noexcept;
    DockTab active_tab() const noexcept { return active_tab_; }

private:
    DockTab probe_tab(int32_t py) const noexcept;
    element_base* get_window_for_tab(sys::state& state, DockTab tab) noexcept;

    DockTab active_tab_ = DockTab::None;
    DockTab hovered_tab_ = DockTab::None;
};

std::unique_ptr<mobile_dock_bar_window> create_mobile_dock_bar(sys::state& state);

} // namespace ui
