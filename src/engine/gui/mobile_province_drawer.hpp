#pragma once
// =============================================================================
// mobile_province_drawer.hpp
// Half-Screen 3-Tab Slide-Out Province Drawer for 20:9 Mobile Screens
// Occupies the right 46% of the screen, leaving the left 54% visible for map
// navigation, tactical unit commands, and territorial comparison.
//
// 3 Dedicated Touch Tabs:
//   - Tab 0: Overview & Military (概览 / 军事: 人口、要塞、海港、驻军与征兵)
//   - Tab 1: RGO & Local Prices (物产 / 物价: 特产、产能、铁路基建与 MAPI 准入)
//   - Tab 2: Factories & Build (建筑 / 投资: 工厂列表、补贴开关与扩建入口)
// =============================================================================

#include "gui_element_types.hpp"
#include "dcon_generated_ids.hpp"
#include "constants_dcon.hpp"
#include "system_state_forward.hpp"
#include <cstdint>
#include <memory>

namespace ui {

enum class ProvinceDrawerTab : uint8_t {
    OverviewMilitary = 0,
    RgoMarket = 1,
    FactoriesBuild = 2,
    Count = 3
};

class mobile_province_drawer_window : public container_base {
public:
    mobile_province_drawer_window() = default;

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

    void open_province(sys::state& state, dcon::province_id prov) noexcept;
    void close_drawer(sys::state& state) noexcept;

    bool is_open() const noexcept { return is_open_; }
    dcon::province_id active_province() const noexcept { return active_province_; }
    ProvinceDrawerTab active_tab() const noexcept { return active_tab_; }

private:
    int32_t probe_drawer_item(int32_t px, int32_t py) const noexcept;

    bool is_open_ = false;
    dcon::province_id active_province_{};
    ProvinceDrawerTab active_tab_ = ProvinceDrawerTab::OverviewMilitary;
    int32_t hovered_item_ = -1;
};

std::unique_ptr<mobile_province_drawer_window> create_mobile_province_drawer(sys::state& state);

} // namespace ui
