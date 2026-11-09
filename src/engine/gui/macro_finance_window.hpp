#pragma once

#include "gui_element_types.hpp"
#include "system_state_forward.hpp"
#include <memory>
#include <string>
#include <vector>

namespace ui {

class macro_finance_window : public window_element_base {
public:
	int32_t current_tab = 0; // 0: 央行政策, 1: 国资控股, 2: 股票市场

	macro_finance_window();
	~macro_finance_window() override = default;

	void on_create(sys::state& state) noexcept override;
	void on_update(sys::state& state) noexcept override;
	void render(sys::state& state, int32_t x, int32_t y) noexcept override;
	message_result test_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type type) noexcept override;

	void rebuild_tab_controls(sys::state& state);
	void switch_tab(sys::state& state, int32_t tab_index);
};

// Global toggle helper
void toggle_macro_finance_window(sys::state& state);

// Floating Screen Trigger Button (Accessible directly on mobile touch screen & desktop)
class macro_finance_trigger_button : public button_element_base {
public:
	macro_finance_trigger_button();
	void on_create(sys::state& state) noexcept override;
	void on_update(sys::state& state) noexcept override;
	void button_action(sys::state& state) noexcept override;
	void render(sys::state& state, int32_t x, int32_t y) noexcept override;
	tooltip_behavior has_tooltip(sys::state& state) noexcept override;
	void update_tooltip(sys::state& state, int32_t x, int32_t y, text::columnar_layout& contents) noexcept override;
};

// Minimap toolbar button
class minimap_macro_finance_button : public button_element_base {
public:
	void button_action(sys::state& state) noexcept override;
	tooltip_behavior has_tooltip(sys::state& state) noexcept override;
	void update_tooltip(sys::state& state, int32_t x, int32_t y, text::columnar_layout& contents) noexcept override;
};

} // namespace ui
