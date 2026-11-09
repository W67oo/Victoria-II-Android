#include "macro_finance_window.hpp"
#include "system_state.hpp"
#include "opengl_wrapper.hpp"
#include "text.hpp"
#include "central_bank.hpp"
#include "dual_ownership.hpp"
#include "stock_exchange.hpp"
#include "sound.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <functional>

namespace ui {

namespace {

void draw_text_helper(sys::state& state, int32_t x, int32_t y, int32_t w, int32_t h, uint16_t font_id, std::string_view str, ogl::color3f const& col, text::alignment align = text::alignment::left) {
	if(font_id == 0) {
		font_id = text::name_into_font_id(state, "Main_14");
	}
	text::layout layout;
	text::single_line_layout sl{
		layout,
		text::layout_parameters{
			0, 0,
			static_cast<int16_t>(w),
			static_cast<int16_t>(h),
			font_id, 0,
			align,
			text::text_color::white,
			true, true
		},
		state.world.locale_get_native_rtl(state.font_collection.get_current_locale()) ? text::layout_base::rtl_status::rtl : text::layout_base::rtl_status::ltr
	};
	sl.add_text(state, str);
	for(auto& t : layout.contents) {
		ui::render_text_chunk(state, t, float(x) + t.x, float(y) + t.y, font_id, col, ogl::color_modification::none);
	}
}

class macro_finance_btn : public button_element_base {
public:
	std::function<void(sys::state&)> callback;
	std::string label;
	ogl::color3f bg_col{0.20f, 0.25f, 0.32f};
	ogl::color3f text_col{1.0f, 1.0f, 1.0f};

	macro_finance_btn(std::string text_label, int16_t px, int16_t py, int16_t sx, int16_t sy, std::function<void(sys::state&)> cb, ogl::color3f bg = {0.20f, 0.25f, 0.32f})
		: callback(std::move(cb)), label(std::move(text_label)), bg_col(bg) {
		base_data.flags = (base_data.flags & ~element_data::type_mask) | uint8_t(element_type::button);
		base_data.position = xy_pair{ px, py };
		base_data.size = xy_pair{ sx, sy };
		interactable = true;
	}

	void button_action(sys::state& state) noexcept override {
		if(callback) {
			callback(state);
			if(parent) {
				parent->impl_on_update(state);
			}
		}
	}

	void render(sys::state& state, int32_t x, int32_t y) noexcept override {
		bool hover = (this == state.ui_state.under_mouse);
		float r = hover ? std::min(1.0f, bg_col.r * 1.35f) : bg_col.r;
		float g = hover ? std::min(1.0f, bg_col.g * 1.35f) : bg_col.g;
		float b = hover ? std::min(1.0f, bg_col.b * 1.35f) : bg_col.b;

		// Button background
		ogl::render_alpha_colored_rect(state, float(x), float(y), float(base_data.size.x), float(base_data.size.y), r, g, b, 1.0f);

		// Button bevel border
		float border_alpha = hover ? 0.95f : 0.65f;
		ogl::render_alpha_colored_rect(state, float(x), float(y), float(base_data.size.x), 1.0f, 0.55f, 0.65f, 0.80f, border_alpha);
		ogl::render_alpha_colored_rect(state, float(x), float(y + base_data.size.y - 1), float(base_data.size.x), 1.0f, 0.08f, 0.10f, 0.14f, border_alpha);
		ogl::render_alpha_colored_rect(state, float(x), float(y), 1.0f, float(base_data.size.y), 0.55f, 0.65f, 0.80f, border_alpha);
		ogl::render_alpha_colored_rect(state, float(x + base_data.size.x - 1), float(y), 1.0f, float(base_data.size.y), 0.08f, 0.10f, 0.14f, border_alpha);

		// Button text
		uint16_t fid = base_data.data.button.font_handle;
		if(fid == 0) fid = text::name_into_font_id(state, "Main_14");
		draw_text_helper(state, x, y + 5, base_data.size.x, base_data.size.y, fid, label, text_col, text::alignment::center);
	}
};

} // namespace

macro_finance_window::macro_finance_window() {
	base_data.flags = (base_data.flags & ~element_data::type_mask) | uint8_t(element_type::window);
	base_data.size = xy_pair{ 580, 420 };
	base_data.data.window.flags |= window_data::is_moveable_mask;
}

void macro_finance_window::on_create(sys::state& state) noexcept {
	window_element_base::on_create(state);
	base_data.position.x = int16_t(std::max(10, (int32_t(state.x_size) - 580) / 2));
	base_data.position.y = int16_t(std::max(40, (int32_t(state.y_size) - 420) / 2));
	rebuild_tab_controls(state);
}

void macro_finance_window::on_update(sys::state& state) noexcept {
	window_element_base::on_update(state);
}

message_result macro_finance_window::test_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type type) noexcept {
	return message_result::consumed;
}

void macro_finance_window::switch_tab(sys::state& state, int32_t tab_index) {
	current_tab = std::clamp(tab_index, 0, 2);
	rebuild_tab_controls(state);
}

void macro_finance_window::rebuild_tab_controls(sys::state& state) {
	children.clear();

	// 1. Close button [X]
	auto close_btn = std::make_unique<macro_finance_btn>(
		"X", 546, 5, 26, 22,
		[](sys::state& s) {
			if(s.ui_state.macro_finance_window) {
				s.ui_state.macro_finance_window->set_visible(s, false);
			}
		},
		ogl::color3f{0.50f, 0.15f, 0.15f}
	);
	add_child_to_front(std::move(close_btn));

	// 2. Tab Navigation Buttons
	auto tab0 = std::make_unique<macro_finance_btn>(
		"🏛️ 央行政策", 16, 36, 175, 26,
		[this](sys::state& s) { switch_tab(s, 0); },
		current_tab == 0 ? ogl::color3f{0.25f, 0.38f, 0.55f} : ogl::color3f{0.16f, 0.20f, 0.26f}
	);
	add_child_to_front(std::move(tab0));

	auto tab1 = std::make_unique<macro_finance_btn>(
		"🏭 国资控股", 201, 36, 175, 26,
		[this](sys::state& s) { switch_tab(s, 1); },
		current_tab == 1 ? ogl::color3f{0.25f, 0.38f, 0.55f} : ogl::color3f{0.16f, 0.20f, 0.26f}
	);
	add_child_to_front(std::move(tab1));

	auto tab2 = std::make_unique<macro_finance_btn>(
		"📈 资本市场", 386, 36, 175, 26,
		[this](sys::state& s) { switch_tab(s, 2); },
		current_tab == 2 ? ogl::color3f{0.25f, 0.38f, 0.55f} : ogl::color3f{0.16f, 0.20f, 0.26f}
	);
	add_child_to_front(std::move(tab2));

	dcon::nation_id const player_nat = state.local_player_nation;

	// 3. Tab-Specific Functional Actions
	if(current_tab == 0) {
		// Central Bank Controls
		// Rate cut (-0.25%)
		auto cut_btn = std::make_unique<macro_finance_btn>(
			"利率 -0.25%", 24, 365, 160, 32,
			[player_nat](sys::state& s) {
				if(!player_nat) return;
				auto& cb = economy::get_central_bank_mut(s, player_nat);
				cb.base_interest_rate = std::max(0.005f, cb.base_interest_rate - 0.0025f);
				cb.bond_yield = cb.base_interest_rate + economy::get_credit_spread(cb.credit_rating);
			},
			ogl::color3f{0.18f, 0.35f, 0.25f}
		);
		add_child_to_front(std::move(cut_btn));

		// Rate hike (+0.25%)
		auto hike_btn = std::make_unique<macro_finance_btn>(
			"利率 +0.25%", 204, 365, 160, 32,
			[player_nat](sys::state& s) {
				if(!player_nat) return;
				auto& cb = economy::get_central_bank_mut(s, player_nat);
				cb.base_interest_rate = std::min(0.30f, cb.base_interest_rate + 0.0025f);
				cb.bond_yield = cb.base_interest_rate + economy::get_credit_spread(cb.credit_rating);
			},
			ogl::color3f{0.45f, 0.25f, 0.18f}
		);
		add_child_to_front(std::move(hike_btn));

		// Taylor Rule Reset
		auto taylor_btn = std::make_unique<macro_finance_btn>(
			"泰勒规则均衡", 384, 365, 172, 32,
			[player_nat](sys::state& s) {
				if(!player_nat) return;
				auto& cb = economy::get_central_bank_mut(s, player_nat);
				cb.base_interest_rate = std::clamp(0.02f + 1.5f * cb.inflation_rate, 0.01f, 0.25f);
				cb.bond_yield = cb.base_interest_rate + economy::get_credit_spread(cb.credit_rating);
			},
			ogl::color3f{0.22f, 0.28f, 0.45f}
		);
		add_child_to_front(std::move(taylor_btn));
	}
	else if(current_tab == 1) {
		// State Enterprise & Equity Controls
		// Nationalize +10%
		auto nat_btn = std::make_unique<macro_finance_btn>(
			"推进国资扩股 (+10%)", 24, 365, 170, 32,
			[player_nat](sys::state& s) {
				if(!player_nat) return;
				s.world.for_each_factory([&](dcon::factory_id f) {
					auto loc = s.world.factory_get_province_from_factory_location(f);
					if(loc && s.world.province_get_nation_from_province_ownership(loc) == player_nat) {
						float cur = economy::dual_ownership::get_factory_state_ownership(s, f);
						economy::dual_ownership::set_factory_state_ownership(s, f, std::min(1.0f, cur + 0.10f));
					}
				});
			},
			ogl::color3f{0.20f, 0.35f, 0.40f}
		);
		add_child_to_front(std::move(nat_btn));

		// Privatize -10%
		auto priv_btn = std::make_unique<macro_finance_btn>(
			"国企私有减持 (-10%)", 204, 365, 170, 32,
			[player_nat](sys::state& s) {
				if(!player_nat) return;
				float raised_capital = 0.0f;
				s.world.for_each_factory([&](dcon::factory_id f) {
					auto loc = s.world.factory_get_province_from_factory_location(f);
					if(loc && s.world.province_get_nation_from_province_ownership(loc) == player_nat) {
						float cur = economy::dual_ownership::get_factory_state_ownership(s, f);
						float reduction = std::min(cur, 0.10f);
						if(reduction > 0.0f) {
							economy::dual_ownership::set_factory_state_ownership(s, f, cur - reduction);
							float size = std::max(1.0f, s.world.factory_get_size(f));
							raised_capital += reduction * size * 1000.0f;
						}
					}
				});
				if(raised_capital > 0.0f) {
					float old_t = s.world.nation_get_treasury(player_nat);
					s.world.nation_set_treasury(player_nat, old_t + raised_capital);
				}
			},
			ogl::color3f{0.35f, 0.30f, 0.20f}
		);
		add_child_to_front(std::move(priv_btn));

		// All State 100% / Private Reset
		auto full_btn = std::make_unique<macro_finance_btn>(
			"全面国营化 (100%)", 384, 365, 172, 32,
			[player_nat](sys::state& s) {
				if(!player_nat) return;
				s.world.for_each_factory([&](dcon::factory_id f) {
					auto loc = s.world.factory_get_province_from_factory_location(f);
					if(loc && s.world.province_get_nation_from_province_ownership(loc) == player_nat) {
						economy::dual_ownership::set_factory_state_ownership(s, f, 1.0f);
					}
				});
			},
			ogl::color3f{0.45f, 0.20f, 0.20f}
		);
		add_child_to_front(std::move(full_btn));
	}
	else if(current_tab == 2) {
		// Stock Market & IPO Controls
		// Conduct IPO
		auto ipo_btn = std::make_unique<macro_finance_btn>(
			"承销优质企业 IPO", 24, 365, 170, 32,
			[player_nat](sys::state& s) {
				if(!player_nat) return;
				s.world.for_each_factory([&](dcon::factory_id f) {
					auto loc = s.world.factory_get_province_from_factory_location(f);
					if(loc && s.world.province_get_nation_from_province_ownership(loc) == player_nat) {
						auto quote = economy::stock_market::get_factory_quote(s, f);
						if(!quote.is_public && !quote.in_distress) {
							economy::stock_market::conduct_ipo(s, f);
						}
					}
				});
			},
			ogl::color3f{0.20f, 0.38f, 0.30f}
		);
		add_child_to_front(std::move(ipo_btn));

		// Delist Distressed
		auto delist_btn = std::make_unique<macro_finance_btn>(
			"严重亏损企业退市", 204, 365, 170, 32,
			[player_nat](sys::state& s) {
				if(!player_nat) return;
				s.world.for_each_factory([&](dcon::factory_id f) {
					auto loc = s.world.factory_get_province_from_factory_location(f);
					if(loc && s.world.province_get_nation_from_province_ownership(loc) == player_nat) {
						auto quote = economy::stock_market::get_factory_quote(s, f);
						if(quote.is_public && quote.in_distress) {
							economy::stock_market::delist_factory(s, f);
						}
					}
				});
			},
			ogl::color3f{0.40f, 0.22f, 0.22f}
		);
		add_child_to_front(std::move(delist_btn));

		// Market Relief Injection
		auto relief_btn = std::make_unique<macro_finance_btn>(
			"紧急平准流动性", 384, 365, 172, 32,
			[player_nat](sys::state& s) {
				if(!player_nat) return;
				float cur_t = s.world.nation_get_treasury(player_nat);
				if(cur_t >= 2000.0f) {
					s.world.nation_set_treasury(player_nat, cur_t - 2000.0f);
					// Re-stabilize market index
					auto metrics = economy::stock_market::get_national_metrics(s, player_nat);
					(void)metrics;
				}
			},
			ogl::color3f{0.25f, 0.25f, 0.45f}
		);
		add_child_to_front(std::move(relief_btn));
	}
}

void macro_finance_window::render(sys::state& state, int32_t x, int32_t y) noexcept {
	// 1. Window Main Panel Background (Dark slate with subtle opacity)
	ogl::render_alpha_colored_rect(state, float(x), float(y), float(base_data.size.x), float(base_data.size.y), 0.11f, 0.13f, 0.17f, 0.96f);

	// 2. Title Bar
	ogl::render_alpha_colored_rect(state, float(x), float(y), float(base_data.size.x), 32.0f, 0.16f, 0.20f, 0.26f, 1.0f);
	// Gold accent line
	ogl::render_alpha_colored_rect(state, float(x), float(y + 31), float(base_data.size.x), 2.0f, 0.85f, 0.72f, 0.25f, 1.0f);

	// Title text
	uint16_t fid = text::name_into_font_id(state, "Main_14");
	draw_text_helper(state, x + 16, y + 7, base_data.size.x - 60, 24, fid, "宏观金融与资本市场管理 (Macro-Finance & Capital Markets)", ogl::color3f{0.95f, 0.92f, 0.80f});

	// 3. Tab Divider Line
	ogl::render_alpha_colored_rect(state, float(x + 16), float(y + 64), float(base_data.size.x - 32), 1.0f, 0.30f, 0.36f, 0.45f, 0.7f);

	// 4. Content Area Cards
	dcon::nation_id const player_nat = state.local_player_nation;

	if(current_tab == 0) {
		// =========================================================================
		// Tab 0: 央行与信用 (Central Bank & Sovereign Rating)
		// =========================================================================
		auto const& cb = economy::get_central_bank(state, player_nat);

		// Card 1: 货币制度与储备 (Reserve & Parity)
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 75), float(base_data.size.x - 40), 125.0f, 0.14f, 0.17f, 0.22f, 0.9f);
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 75), float(base_data.size.x - 40), 22.0f, 0.18f, 0.22f, 0.28f, 1.0f);
		draw_text_helper(state, x + 30, y + 78, 300, 20, fid, "【国家货币制度与金银外汇储备】", ogl::color3f{0.75f, 0.85f, 0.95f});

		const char* std_name = "古典金本位制 (Gold Standard)";
		if(cb.standard == economy::currency_standard::silver_standard) std_name = "白银本位制 (Silver Standard)";
		else if(cb.standard == economy::currency_standard::bimetallic_standard) std_name = "金银复本位制 (Bimetallism)";
		else if(cb.standard == economy::currency_standard::fiat_standard) std_name = "信用纸币本位制 (Fiat Paper)";

		char buf[256];
		std::snprintf(buf, sizeof(buf), "货币本位制度: %s", std_name);
		draw_text_helper(state, x + 32, y + 104, 500, 18, fid, buf, ogl::color3f{0.90f, 0.90f, 0.90f});

		std::snprintf(buf, sizeof(buf), "央行贵金属库存储备: £%.1f  |  币种代号: %s (%s)", cb.bullion_reserve, economy::get_currency_symbol(state, player_nat), economy::get_currency_name(state, player_nat));
		draw_text_helper(state, x + 32, y + 126, 500, 18, fid, buf, ogl::color3f{0.90f, 0.85f, 0.40f});

		std::snprintf(buf, sizeof(buf), "购买力平价汇率 (PPP vs £): 1.00 : %.4f  |  国际收支净额: £%.1f/周", cb.exchange_rate_vs_anchor, cb.weekly_bop_balance);
		draw_text_helper(state, x + 32, y + 148, 500, 18, fid, buf, ogl::color3f{0.70f, 0.85f, 0.70f});

		std::snprintf(buf, sizeof(buf), "物价指数 (CPI 基期 1.0): %.3f  |  年化通货膨胀率: %+.2f%%", cb.price_level_index, cb.inflation_rate * 100.0f);
		draw_text_helper(state, x + 32, y + 170, 500, 18, fid, buf, cb.inflation_rate > 0.05f ? ogl::color3f{0.95f, 0.40f, 0.40f} : ogl::color3f{0.70f, 0.90f, 0.90f});

		// Card 2: 利率走廊与主权信用 (Interest Rate & Bond Yield)
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 215), float(base_data.size.x - 40), 135.0f, 0.14f, 0.17f, 0.22f, 0.9f);
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 215), float(base_data.size.x - 40), 22.0f, 0.18f, 0.22f, 0.28f, 1.0f);
		draw_text_helper(state, x + 30, y + 218, 300, 20, fid, "【央行贴现利率与主权公债收益率】", ogl::color3f{0.75f, 0.85f, 0.95f});

		std::snprintf(buf, sizeof(buf), "央行基准政策贴现利率: %.2f%%", cb.base_interest_rate * 100.0f);
		draw_text_helper(state, x + 32, y + 244, 500, 18, fid, buf, ogl::color3f{0.40f, 0.90f, 0.80f});

		std::snprintf(buf, sizeof(buf), "国家主权信用等级: [%s] 级  |  国际信用利差: +%.2f%%", economy::get_rating_str(cb.credit_rating), economy::get_credit_spread(cb.credit_rating) * 100.0f);
		draw_text_helper(state, x + 32, y + 266, 500, 18, fid, buf, ogl::color3f{0.95f, 0.80f, 0.35f});

		std::snprintf(buf, sizeof(buf), "长期国债实际借贷利率 (基准+利差): %.2f%%", cb.bond_yield * 100.0f);
		draw_text_helper(state, x + 32, y + 288, 500, 18, fid, buf, ogl::color3f{0.90f, 0.90f, 0.50f});

		std::snprintf(buf, sizeof(buf), "提示: 降息可扩张工业投资，加息可平抑物价通胀，提高信用评级能大幅降低财政偿债负担。");
		draw_text_helper(state, x + 32, y + 312, 520, 18, fid, buf, ogl::color3f{0.65f, 0.70f, 0.75f});
	}
	else if(current_tab == 1) {
		// =========================================================================
		// Tab 1: 国资控股与分红 (State Enterprise & Dividends)
		// =========================================================================
		float daily_div = economy::dual_ownership::get_daily_state_dividends(player_nat);
		uint32_t total_factories = 0;
		uint32_t state_factories = 0;
		float sum_state_share = 0.0f;

		state.world.for_each_factory([&](dcon::factory_id f) {
			auto loc = state.world.factory_get_province_from_factory_location(f);
			if(loc && state.world.province_get_nation_from_province_ownership(loc) == player_nat) {
				total_factories++;
				float share = economy::dual_ownership::get_factory_state_ownership(state, f);
				sum_state_share += share;
				if(share > 0.01f) {
					state_factories++;
				}
			}
		});

		float avg_share = total_factories > 0 ? (sum_state_share / float(total_factories)) : 0.0f;

		// Card 1: 国资分红与工业底盘
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 75), float(base_data.size.x - 40), 125.0f, 0.14f, 0.17f, 0.22f, 0.9f);
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 75), float(base_data.size.x - 40), 22.0f, 0.18f, 0.22f, 0.28f, 1.0f);
		draw_text_helper(state, x + 30, y + 78, 300, 20, fid, "【国有资产资本池与工业股权配置】", ogl::color3f{0.75f, 0.85f, 0.95f});

		char buf[256];
		std::snprintf(buf, sizeof(buf), "昨日国资利润分红入库 (划转财政): £%.2f", daily_div);
		draw_text_helper(state, x + 32, y + 104, 500, 18, fid, buf, ogl::color3f{0.40f, 0.95f, 0.50f});

		std::snprintf(buf, sizeof(buf), "全境工业工厂总数: %u 座  |  拥有国有股份企业: %u 座", total_factories, state_factories);
		draw_text_helper(state, x + 32, y + 126, 500, 18, fid, buf, ogl::color3f{0.90f, 0.90f, 0.90f});

		std::snprintf(buf, sizeof(buf), "全国工业平均国有持股比例: %.1f%%  |  民营资本持股: %.1f%%", avg_share * 100.0f, (1.0f - avg_share) * 100.0f);
		draw_text_helper(state, x + 32, y + 148, 500, 18, fid, buf, ogl::color3f{0.85f, 0.75f, 0.35f});

		std::snprintf(buf, sizeof(buf), "国资运作机制: 国企净利润按股比每日向国家财政分红，民资利润注入资本家投资池。");
		draw_text_helper(state, x + 32, y + 170, 520, 18, fid, buf, ogl::color3f{0.70f, 0.80f, 0.85f});

		// Card 2: 产权结构调节与指令
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 215), float(base_data.size.x - 40), 135.0f, 0.14f, 0.17f, 0.22f, 0.9f);
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 215), float(base_data.size.x - 40), 22.0f, 0.18f, 0.22f, 0.28f, 1.0f);
		draw_text_helper(state, x + 30, y + 218, 300, 20, fid, "【产权结构调整策略与财政回笼】", ogl::color3f{0.75f, 0.85f, 0.95f});

		std::snprintf(buf, sizeof(buf), "当前工业战略方向: %s", avg_share > 0.5f ? "国有主导型工业体系 (State-Dominant)" : "混合所有制与民营主导 (Mixed/Private)");
		draw_text_helper(state, x + 32, y + 244, 500, 18, fid, buf, ogl::color3f{0.95f, 0.85f, 0.40f});

		std::snprintf(buf, sizeof(buf), "【推进扩股】: 提升国家持股，增强财政未来经常性分红收入。");
		draw_text_helper(state, x + 32, y + 266, 500, 18, fid, buf, ogl::color3f{0.80f, 0.85f, 0.90f});

		std::snprintf(buf, sizeof(buf), "【私有减持】: 减持国有股至民营投资池，即时向国库回笼大额现金资产。");
		draw_text_helper(state, x + 32, y + 288, 500, 18, fid, buf, ogl::color3f{0.80f, 0.85f, 0.90f});

		std::snprintf(buf, sizeof(buf), "【全面国营】: 一键将全境工业收归国有，彻底集中工业利润。");
		draw_text_helper(state, x + 32, y + 310, 500, 18, fid, buf, ogl::color3f{0.95f, 0.60f, 0.60f});
	}
	else if(current_tab == 2) {
		// =========================================================================
		// Tab 2: 股票市场与IPO (Stock Market & Capital)
		// =========================================================================
		auto metrics = economy::stock_market::get_national_metrics(state, player_nat);

		// Card 1: 资本市场全景
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 75), float(base_data.size.x - 40), 125.0f, 0.14f, 0.17f, 0.22f, 0.9f);
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 75), float(base_data.size.x - 40), 22.0f, 0.18f, 0.22f, 0.28f, 1.0f);
		draw_text_helper(state, x + 30, y + 78, 300, 20, fid, "【全国证券交易所行情与市值概览】", ogl::color3f{0.75f, 0.85f, 0.95f});

		char buf[256];
		std::snprintf(buf, sizeof(buf), "股票综合指数: %.2f 点 (基期 1000.0)  |  交易所挂牌企业: %u 家", metrics.composite_index, metrics.listed_companies);
		draw_text_helper(state, x + 32, y + 104, 500, 18, fid, buf, metrics.composite_index >= 1000.0f ? ogl::color3f{0.40f, 0.95f, 0.50f} : ogl::color3f{0.95f, 0.40f, 0.40f});

		std::snprintf(buf, sizeof(buf), "上市公司股票总市值: £%.1f  |  市场平均市盈率 (P/E): %.1f 倍", metrics.total_market_cap, metrics.average_pe);
		draw_text_helper(state, x + 32, y + 126, 500, 18, fid, buf, ogl::color3f{0.90f, 0.85f, 0.40f});

		const char* crunch_desc = metrics.credit_crunch_active ? "⚠️ 发生系统性信贷紧缩危机 (Credit Crunch Freeze)" : "正常平稳 (Normal Liquidity)";
		std::snprintf(buf, sizeof(buf), "银行流动性信贷状态: %s", crunch_desc);
		draw_text_helper(state, x + 32, y + 148, 500, 18, fid, buf, metrics.credit_crunch_active ? ogl::color3f{0.95f, 0.35f, 0.35f} : ogl::color3f{0.50f, 0.90f, 0.60f});

		std::snprintf(buf, sizeof(buf), "流动性紧缩严重程度: %.1f%%  |  高危濒危企业: %u 家", metrics.credit_crunch_severity * 100.0f, metrics.distressed_companies);
		draw_text_helper(state, x + 32, y + 170, 500, 18, fid, buf, metrics.distressed_companies > 0 ? ogl::color3f{0.95f, 0.50f, 0.40f} : ogl::color3f{0.70f, 0.85f, 0.90f});

		// Card 2: 资本市场调控与纾困
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 215), float(base_data.size.x - 40), 135.0f, 0.14f, 0.17f, 0.22f, 0.9f);
		ogl::render_alpha_colored_rect(state, float(x + 20), float(y + 215), float(base_data.size.x - 40), 22.0f, 0.18f, 0.22f, 0.28f, 1.0f);
		draw_text_helper(state, x + 30, y + 218, 300, 20, fid, "【资本市场承销、退市及流动性平准】", ogl::color3f{0.75f, 0.85f, 0.95f});

		std::snprintf(buf, sizeof(buf), "【企业 IPO】: 协助未上市优质工业企业在主板公开发行股票，吸纳民间储蓄。");
		draw_text_helper(state, x + 32, y + 244, 500, 18, fid, buf, ogl::color3f{0.80f, 0.85f, 0.90f});

		std::snprintf(buf, sizeof(buf), "【停牌退市】: 强制清除资不抵债严重亏损的僵尸企业，避免拖垮指数。");
		draw_text_helper(state, x + 32, y + 266, 500, 18, fid, buf, ogl::color3f{0.80f, 0.85f, 0.90f});

		std::snprintf(buf, sizeof(buf), "【平准注资】: 消耗 £2,000 国库资金向市场注入紧急流动性，缓解挤兑恐慌。");
		draw_text_helper(state, x + 32, y + 288, 500, 18, fid, buf, ogl::color3f{0.90f, 0.80f, 0.45f});

		std::snprintf(buf, sizeof(buf), "提示: 繁荣的股市可提升全国 POP 购买力与工业扩张效率，崩盘则会导致投资锐减。");
		draw_text_helper(state, x + 32, y + 312, 520, 18, fid, buf, ogl::color3f{0.65f, 0.70f, 0.75f});
	}

	// 5. Border Frame
	ogl::render_alpha_colored_rect(state, float(x), float(y), float(base_data.size.x), 1.0f, 0.40f, 0.50f, 0.65f, 0.8f);
	ogl::render_alpha_colored_rect(state, float(x), float(y + base_data.size.y - 1), float(base_data.size.x), 1.0f, 0.15f, 0.18f, 0.24f, 0.8f);
	ogl::render_alpha_colored_rect(state, float(x), float(y), 1.0f, float(base_data.size.y), 0.40f, 0.50f, 0.65f, 0.8f);
	ogl::render_alpha_colored_rect(state, float(x + base_data.size.x - 1), float(y), 1.0f, float(base_data.size.y), 0.15f, 0.18f, 0.24f, 0.8f);

	// Children (buttons) render automatically via container_base::impl_render
}

void toggle_macro_finance_window(sys::state& state) {
	if(!state.ui_state.macro_finance_window) {
		auto win = std::make_unique<macro_finance_window>();
		win->on_create(state);
		state.ui_state.macro_finance_window = win.get();
		state.ui_state.macro_finance_window->set_visible(state, true);
		state.ui_state.root->add_child_to_front(std::move(win));
	} else if(state.ui_state.macro_finance_window->is_visible()) {
		state.ui_state.macro_finance_window->set_visible(state, false);
	} else {
		state.ui_state.macro_finance_window->set_visible(state, true);
		state.ui_state.root->move_child_to_front(state.ui_state.macro_finance_window);
		state.ui_state.macro_finance_window->impl_on_update(state);
	}
}

// =============================================================================
// macro_finance_trigger_button (Floating screen trigger button)
// =============================================================================
macro_finance_trigger_button::macro_finance_trigger_button() {
	base_data.flags = (base_data.flags & ~element_data::type_mask) | uint8_t(element_type::button);
	base_data.size = xy_pair{ 98, 26 };
	interactable = true;
}

void macro_finance_trigger_button::on_create(sys::state& state) noexcept {
	base_data.position.x = int16_t(std::max(0, int32_t(state.x_size) - 106));
	base_data.position.y = 40;
}

void macro_finance_trigger_button::on_update(sys::state& state) noexcept {
	base_data.position.x = int16_t(std::max(0, int32_t(state.x_size) - 106));
	base_data.position.y = 40;
}

void macro_finance_trigger_button::button_action(sys::state& state) noexcept {
	toggle_macro_finance_window(state);
}

void macro_finance_trigger_button::render(sys::state& state, int32_t x, int32_t y) noexcept {
	bool hover = (this == state.ui_state.under_mouse);
	float r = hover ? 0.32f : 0.18f;
	float g = hover ? 0.42f : 0.24f;
	float b = hover ? 0.55f : 0.32f;

	// Button fill
	ogl::render_alpha_colored_rect(state, float(x), float(y), float(base_data.size.x), float(base_data.size.y), r, g, b, 0.95f);

	// Gold highlight border
	ogl::render_alpha_colored_rect(state, float(x), float(y), float(base_data.size.x), 1.0f, 0.85f, 0.72f, 0.30f, 0.9f);
	ogl::render_alpha_colored_rect(state, float(x), float(y + base_data.size.y - 1), float(base_data.size.x), 1.0f, 0.50f, 0.40f, 0.15f, 0.9f);
	ogl::render_alpha_colored_rect(state, float(x), float(y), 1.0f, float(base_data.size.y), 0.85f, 0.72f, 0.30f, 0.9f);
	ogl::render_alpha_colored_rect(state, float(x + base_data.size.x - 1), float(y), 1.0f, float(base_data.size.y), 0.50f, 0.40f, 0.15f, 0.9f);

	// Button text
	uint16_t fid = base_data.data.button.font_handle;
	if(fid == 0) fid = text::name_into_font_id(state, "Main_14");
	draw_text_helper(state, x, y + 5, base_data.size.x, base_data.size.y, fid, "🏛️ 金融菜单", ogl::color3f{0.95f, 0.92f, 0.80f}, text::alignment::center);
}

tooltip_behavior macro_finance_trigger_button::has_tooltip(sys::state& state) noexcept {
	return tooltip_behavior::tooltip;
}

void macro_finance_trigger_button::update_tooltip(sys::state& state, int32_t x, int32_t y, text::columnar_layout& contents) noexcept {
	auto box = text::open_layout_box(contents, 0);
	text::add_to_layout_box(state, contents, box, std::string_view("宏观金融与资本市场管理 (央行利率/主权评级/国资分红/股市IPO)"));
	text::close_layout_box(contents, box);
}

// =============================================================================
// minimap_macro_finance_button
// =============================================================================
void minimap_macro_finance_button::button_action(sys::state& state) noexcept {
	toggle_macro_finance_window(state);
}

tooltip_behavior minimap_macro_finance_button::has_tooltip(sys::state& state) noexcept {
	return tooltip_behavior::tooltip;
}

void minimap_macro_finance_button::update_tooltip(sys::state& state, int32_t x, int32_t y, text::columnar_layout& contents) noexcept {
	auto box = text::open_layout_box(contents, 0);
	text::add_to_layout_box(state, contents, box, std::string_view("宏观金融与资本市场"));
	text::close_layout_box(contents, box);
}

} // namespace ui
