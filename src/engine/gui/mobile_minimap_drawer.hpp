#pragma once
// =============================================================================
// mobile_minimap_drawer.hpp
// Collapsible Minimap & Mapmode Selector Tray for 20:9 Mobile Screens
// Features:
//   - Collapsed by default into a 48dp brass compass floating anchor
//   - Tapping expands the minimap radar tray with 8 primary mapmodes & search/ledger
//   - Slashes bottom-right screen clutter on touch devices
// =============================================================================

#include "gui_element_types.hpp"
#include "map_modes.hpp"
#include "system_state_forward.hpp"
#include <cstdint>
#include <memory>

namespace ui {

class mobile_minimap_drawer_window : public container_base {
public:
    mobile_minimap_drawer_window() = default;

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

    void set_expanded(bool exp) noexcept { is_expanded_ = exp; }
    bool is_expanded() const noexcept { return is_expanded_; }

private:
    int32_t probe_sub_item(int32_t px, int32_t py) const noexcept;

    bool is_expanded_ = false;
    int32_t hovered_item_ = -1;
};

std::unique_ptr<mobile_minimap_drawer_window> create_mobile_minimap_drawer(sys::state& state);

} // namespace ui
