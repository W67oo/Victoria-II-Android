#include "mobile_topbar.hpp"
#include "system_state.hpp"
#include "nations.hpp"
#include "culture.hpp"
#include "sound.hpp"
#include "nine_slice_renderer.hpp"
#include "runtime_config_android.hpp"
#include "central_bank.hpp"
#include "stock_exchange.hpp"
#include "dual_ownership.hpp"
#include "gui_ledger_window.hpp"
#include <fmt/format.h>
#include <algorithm>

namespace ui {

void mobile_topbar_window::on_create(sys::state& state) noexcept {
    container_base::on_create(state);
    base_data.position = xy_pair{ 0, 0 };
    base_data.size = xy_pair{ int16_t(state.x_size), 44 };
}

void mobile_topbar_window::impl_on_update(sys::state& state) noexcept {
    base_data.size.x = int16_t(state.x_size);
    base_data.size.y = 44;
    container_base::impl_on_update(state);
}

TopbarSection mobile_topbar_window::probe_section(int32_t px, int32_t py, sys::state& state) const noexcept {
    if (py < 0 || py > 44) return TopbarSection::None;

    vic2_mobile::input::SafeAreaInsets insets{};
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        insets = vic2_mobile::runtime::config().insets;
    }

    int32_t sl = insets.left;
    int32_t sr = insets.right;
    int32_t w = state.x_size;

    // Left-aligned sections
    if (px >= sl && px < sl + 110) return TopbarSection::RankAndFlag;
    if (px >= sl + 115 && px < sl + 230) return TopbarSection::Treasury;
    if (px >= sl + 235 && px < sl + 335) return TopbarSection::InflationCredit;
    if (px >= sl + 340 && px < sl + 500) return TopbarSection::Research;
    if (px >= sl + 505 && px < sl + 640) return TopbarSection::PowerPoints;
    if (px >= sl + 645 && px < sl + 710) return TopbarSection::Newspaper;

    // Right-aligned speed and date sections
    int32_t rx = w - sr;
    if (px >= rx - 50 && px < rx) return TopbarSection::SpeedInc;
    if (px >= rx - 90 && px < rx - 50) return TopbarSection::SpeedDec;
    if (px >= rx - 135 && px < rx - 90) return TopbarSection::SpeedPause;
    if (px >= rx - 250 && px < rx - 135) return TopbarSection::Date;

    return TopbarSection::None;
}

mouse_probe mobile_topbar_window::impl_probe_mouse(sys::state& state, int32_t x, int32_t y, mouse_probe_type type) noexcept {
    if (x >= base_data.position.x && x < base_data.position.x + base_data.size.x &&
        y >= base_data.position.y && y < base_data.position.y + base_data.size.y) {
        hovered_section_ = probe_section(x, y, state);
        return mouse_probe{ this, xy_pair{ int16_t(x), int16_t(y) } };
    }
    return mouse_probe{};
}

message_result mobile_topbar_window::impl_on_lbutton_down(sys::state& /*state*/, int32_t /*x*/, int32_t /*y*/, sys::key_modifiers /*mods*/) noexcept {
    return message_result::consumed;
}

message_result mobile_topbar_window::impl_on_lbutton_up(sys::state& state, int32_t x, int32_t y, sys::key_modifiers /*mods*/, bool under_mouse) noexcept {
    if (!under_mouse) return message_result::unseen;

    auto sec = probe_section(x, y, state);
    switch (sec) {
    case TopbarSection::SpeedPause: {
        if (state.actual_game_speed.load() <= 0) {
            state.actual_game_speed = std::max(1, state.ui_state.held_game_speed);
        } else {
            state.ui_state.held_game_speed = state.actual_game_speed.load();
            state.actual_game_speed = 0;
        }
        return message_result::consumed;
    }
    case TopbarSection::SpeedDec: {
        int cur = state.actual_game_speed.load();
        if (cur > 1) {
            state.actual_game_speed = cur - 1;
            state.ui_state.held_game_speed = cur - 1;
        }
        return message_result::consumed;
    }
    case TopbarSection::SpeedInc: {
        int cur = state.actual_game_speed.load();
        if (cur < 5) {
            state.actual_game_speed = cur + 1;
            state.ui_state.held_game_speed = cur + 1;
        }
        return message_result::consumed;
    }
    case TopbarSection::Research: {
        if (state.ui_state.technology_subwindow) {
            bool vis = state.ui_state.technology_subwindow->is_visible();
            state.ui_state.technology_subwindow->set_visible(state, !vis);
            if (!vis) state.ui_state.root->move_child_to_front(state.ui_state.technology_subwindow);
        }
        return message_result::consumed;
    }
    case TopbarSection::Treasury: {
        if (state.ui_state.topbar_subwindow) {
            bool vis = state.ui_state.topbar_subwindow->is_visible();
            state.ui_state.topbar_subwindow->set_visible(state, !vis);
            if (!vis) state.ui_state.root->move_child_to_front(state.ui_state.topbar_subwindow);
        }
        return message_result::consumed;
    }
        case TopbarSection::InflationCredit: {
        if (!state.ui_state.ledger_window) {
            auto window = make_element_by_type<ledger_window>(state, "ledger");
            state.ui_state.ledger_window = window.get();
        }
        if (state.ui_state.ledger_window) {
            bool vis = state.ui_state.ledger_window->is_visible();
            if (vis) {
                state.ui_state.ledger_window->set_visible(state, false);
            } else {
                state.ui_state.ledger_window->set_visible(state, true);
                state.ui_state.root->move_child_to_front(state.ui_state.ledger_window);
                Cyto::Any payload = ledger_page_number{int8_t(13)};
                state.ui_state.ledger_window->impl_set(state, payload);
            }
        }
        return message_result::consumed;
    }
    case TopbarSection::Newspaper: {
        if (state.ui_state.msg_log_window) {
            bool vis = state.ui_state.msg_log_window->is_visible();
            state.ui_state.msg_log_window->set_visible(state, !vis);
            if (!vis) state.ui_state.root->move_child_to_front(state.ui_state.msg_log_window);
        }
        return message_result::consumed;
    }
    default:
        break;
    }

    return message_result::consumed;
}

void mobile_topbar_window::render(sys::state& state, int32_t x, int32_t y) noexcept {
    vic2_mobile::input::SafeAreaInsets insets{};
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        insets = vic2_mobile::runtime::config().insets;
    }

    int32_t sl = insets.left;
    int32_t sr = insets.right;
    int32_t eff_w = state.x_size - sl - sr;
    int32_t top_h = 44;

    // 1. Render 9-slice brass capsule frame
    ui::render_nine_slice(
        state,
        ui::slice_presets::topbar_capsule,
        float(x + sl),
        float(y),
        float(eff_w),
        float(top_h),
        state.ui_state.bg_gfx_id
    );

    // 2. Render child elements (recursively)
    container_base::render(state, x, y);
}

void mobile_topbar_window::update_tooltip(sys::state& state, int32_t /*x*/, int32_t /*y*/, text::columnar_layout& contents) noexcept {
    dcon::nation_id n = state.local_player_nation;
    if (!state.world.nation_is_valid(n)) return;

    auto box = text::open_layout_box(contents, 0);

    switch (hovered_section_) {
    case TopbarSection::RankAndFlag: {
        int32_t prestige = int32_t(nations::prestige_score(state, n));
        int32_t industry = int32_t(state.world.nation_get_industrial_score(n));
        int32_t military = int32_t(state.world.nation_get_military_score(n));
        text::add_line(state, contents, "rank_prestige");
        text::add_to_layout_box(state, contents, box, fmt::format("Prestige: {} | Industry: {} | Military: {}", prestige, industry, military));
        break;
    }
    case TopbarSection::Treasury: {
        float treasury = state.world.nation_get_treasury(n);
        float delta = treasury - state.world.nation_get_last_treasury(n);
        text::add_to_layout_box(state, contents, box, fmt::format("Treasury: {:.1f} £ (Daily: {:+.1f} £)", treasury, delta));
        break;
    }
    case TopbarSection::InflationCredit: {
        auto const& cb = economy::get_central_bank(state, n);
        auto const& sm = economy::stock_market::get_national_metrics(state, n);
        float state_div = economy::dual_ownership::get_daily_state_dividends(n);

        static constexpr const char* s_rating_names[] = {
            "AAA", "AA", "A", "BBB", "BB", "B", "CCC", "C", "Default"
        };
        const char* rating_str = (static_cast<size_t>(cb.credit_rating) < 9)
            ? s_rating_names[static_cast<size_t>(cb.credit_rating)]
            : "NR";

        std::string crunch_status = sm.credit_crunch_active
            ? fmt::format(" [CRUNCH: {:.0f}%]", sm.credit_crunch_severity * 100.0f)
            : " [Normal]";

        text::add_to_layout_box(state, contents, box,
            fmt::format("Rating: {} | Rate: {:.1f}% | Infl: {:+.1f}% | Stock: {:.0f} pts (P/E: {:.1f}) | StDiv: +{:.1f} £{}",
                rating_str,
                cb.base_interest_rate * 100.0f,
                cb.inflation_rate * 100.0f,
                sm.composite_index,
                sm.average_pe,
                state_div,
                crunch_status
            )
        );
        break;
    }
    case TopbarSection::Research: {
        auto tech = state.world.nation_get_current_research(n);
        if (tech.is_valid()) {
            float rp = state.world.nation_get_research_points(n);
            float cost = culture::effective_technology_rp_cost(state, state.current_date.to_ymd(state.start_date).year, n, tech);
            text::add_to_layout_box(state, contents, box, fmt::format("Researching: {:.1f} / {:.1f} RP ({:.1f}%)", rp, cost, (cost > 0.f ? rp / cost * 100.f : 0.f)));
        } else {
            text::add_to_layout_box(state, contents, box, std::string_view("No Technology Currently Under Research"));
        }
        break;
    }
    case TopbarSection::PowerPoints: {
        float dip = state.world.nation_get_diplomatic_points(n);
        int32_t max_nf = nations::max_national_focuses(state, n);
        text::add_to_layout_box(state, contents, box, fmt::format("Diplomatic Points: {:.1f} | Max National Focuses: {}", dip, max_nf));
        break;
    }
    case TopbarSection::Newspaper: {
        text::add_to_layout_box(state, contents, box, std::string_view("Victoria Daily Chronicle: Global News Archive"));
        break;
    }
    case TopbarSection::Date:
    case TopbarSection::SpeedPause:
    case TopbarSection::SpeedDec:
    case TopbarSection::SpeedInc: {
        int speed = state.actual_game_speed.load();
        text::add_to_layout_box(state, contents, box, fmt::format("Simulation Speed: {}x (Tap Pause/Play or +/-)", speed > 0 ? std::to_string(speed) : "Paused"));
        break;
    }
    default:
        break;
    }

    text::close_layout_box(contents, box);
}

std::unique_ptr<mobile_topbar_window> create_mobile_topbar(sys::state& state) {
    auto topbar = std::make_unique<mobile_topbar_window>();
    topbar->on_create(state);
    return topbar;
}

} // namespace ui
