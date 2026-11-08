#pragma once
// =============================================================================
// mobile_topbar.hpp
// Compact Single-Row 40dp Landscape Topbar for 20:9 Mobile Screens
// Consolidates:
//   - National Flag & GP/SP Rank Badge
//   - Gold Treasury & Daily Balance Delta (+/-)
//   - Inflation % & Sovereign Credit Rating (AAA ~ C)
//   - Active Research Project & Percentage Mini-Progress Bar
//   - Key Power Points Capsules (NF, Diplomatic, Colonial)
//   - Victorian Newspaper / Global Event Archive Badge
//   - Formatted Date, Touch-Friendly Speed Controls (1x~5x) & Pause Toggle
// =============================================================================

#include "gui_element_types.hpp"
#include "system_state_forward.hpp"
#include <cstdint>
#include <memory>

namespace ui {

enum class TopbarSection : uint8_t {
    None = 0,
    RankAndFlag,
    Treasury,
    InflationCredit,
    Research,
    PowerPoints,
    Newspaper,
    SpeedPause,
    SpeedDec,
    SpeedInc,
    Date
};

class mobile_topbar_window : public container_base {
public:
    mobile_topbar_window() = default;

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

private:
    TopbarSection probe_section(int32_t px, int32_t py, sys::state& state) const noexcept;
    TopbarSection hovered_section_ = TopbarSection::None;
};

std::unique_ptr<mobile_topbar_window> create_mobile_topbar(sys::state& state);

} // namespace ui
