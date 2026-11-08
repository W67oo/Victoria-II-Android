// =============================================================================
// tools/headless_sim/main.cpp
// Headless Simulation Runner for Victoria II / Project Alice
// Benchmarking, Multi-Year Economic Simulation, and Invariant Verification
// =============================================================================

#include "system_state.hpp"
#include "serialization.hpp"
#include "parsers_declarations.hpp"
#include "economy.hpp"
#include "economy_stats.hpp"
#include "economy_production.hpp"
#include "trade_network.hpp"
#include "nations.hpp"
#include "simple_fs.hpp"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace {

struct SimOptions {
    uint32_t days = 365;                 // 1 year default
    uint32_t check_interval = 30;        // Monthly snapshot
    std::string scenario_file = "scenario_1836.bin";
    std::string game_data_dir = "game_data";
    std::string csv_file = "economy_baseline.csv";
    bool rebuild_scenario = false;
};

void print_banner() {
    std::cout << "==================================================================\n";
    std::cout << "  Vic2Mobile Host Headless Simulation Runner (C++20, Host x86_64)\n";
    std::cout << "  Phase 3 Economic Overhaul - Invariant Checks & Baseline Profiling\n";
    std::cout << "==================================================================\n\n";
}

void parse_args(int argc, char** argv, SimOptions& opts) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--days" || arg == "-d") && i + 1 < argc) {
            opts.days = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if ((arg == "--scenario" || arg == "-s") && i + 1 < argc) {
            opts.scenario_file = argv[++i];
        } else if ((arg == "--gamedata" || arg == "-g") && i + 1 < argc) {
            opts.game_data_dir = argv[++i];
        } else if ((arg == "--csv" || arg == "-c") && i + 1 < argc) {
            opts.csv_file = argv[++i];
        } else if ((arg == "--interval" || arg == "-i") && i + 1 < argc) {
            opts.check_interval = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--rebuild") {
            opts.rebuild_scenario = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: vic2_headless_sim [options]\n";
            std::cout << "  -d, --days <N>         Number of simulation days (default: 365)\n";
            std::cout << "  -s, --scenario <file>  Scenario bin cache file (default: scenario_1836.bin)\n";
            std::cout << "  -g, --gamedata <dir>   Victoria 2 game data directory (default: game_data)\n";
            std::cout << "  -c, --csv <file>       Output CSV baseline metrics (default: economy_baseline.csv)\n";
            std::cout << "  -i, --interval <N>     Interval days for CSV logging (default: 30)\n";
            std::cout << "  --rebuild              Force rebuilding scenario from game_data\n";
            std::exit(0);
        }
    }
}

struct MonetarySnapshot {
    float total = 0.0f;
    float treasury = 0.0f;
    float bank = 0.0f;
    float investment_pool = 0.0f;
    float factory = 0.0f;
    float pops = 0.0f;
    float artisans = 0.0f;
    float rgo = 0.0f;
    float market = 0.0f;
    float educators = 0.0f;
    float landlords = 0.0f;
    float ports = 0.0f;
    float global_gdp = 0.0f;
};

MonetarySnapshot take_snapshot(sys::state& state) {
    MonetarySnapshot snap;
    state.world.for_each_nation([&](dcon::nation_id n) {
        auto m = economy::breakdown_nation_monetary_structure(state, n);
        snap.total += m.total;
        snap.treasury += m.nation;
        snap.bank += m.bank;
        snap.investment_pool += m.investment_pool;
        snap.factory += m.factory;
        snap.pops += m.pops;
        snap.artisans += m.artisans;
        snap.rgo += m.rgo;
        snap.market += m.market;
        snap.educators += m.educators;
        snap.landlords += m.landlords;
        snap.ports += m.ports;
        snap.global_gdp += std::max(0.0f, economy::gdp::value_nation(state, n));
    });
    return snap;
}

} // namespace

int main(int argc, char** argv) {
    print_banner();

    SimOptions opts;
    parse_args(argc, argv, opts);

    std::cout << "[Config] Simulation Target: " << opts.days << " days (" 
              << (opts.days / 365.25f) << " years)\n";
    std::cout << "[Config] Game Data Root:   " << opts.game_data_dir << "\n";
    std::cout << "[Config] Scenario Cache:   " << opts.scenario_file << "\n";
    std::cout << "[Config] CSV Export Path:  " << opts.csv_file << "\n";
    std::cout << "[Config] Snapshot Period:  " << opts.check_interval << " days\n\n";

    // Allocate game state on heap (approx ~100MB+, too big for stack)
    std::unique_ptr<sys::state> state = std::make_unique<sys::state>();
    state->network_mode = sys::network_mode_type::single_player;
    state->user_settings.autosaves = sys::autosave_frequency::none;

    // Mount game filesystem
    native_string native_gd_path = simple_fs::utf8_to_native(opts.game_data_dir);
    simple_fs::add_root(state->common_fs, native_gd_path);
    simple_fs::add_root(state->common_fs, NATIVE("."));

    native_string native_scenario_path = simple_fs::utf8_to_native(opts.scenario_file);

    bool scenario_loaded = false;
    if (!opts.rebuild_scenario) {
        std::cout << "[Init] Attempting to load cached scenario: " << opts.scenario_file << " ...\n";
        scenario_loaded = sys::try_read_scenario_and_save_file(*state, native_scenario_path);
    }

    if (!scenario_loaded) {
        std::cout << "[Init] Building scenario from game_data directory (HoD 3.04)...\n";
        auto t_build_start = std::chrono::high_resolution_clock::now();
        parsers::error_handler err("");
        state->load_scenario_data(err, sys::year_month_day{ 1836, 1, 1 });
        auto t_build_end = std::chrono::high_resolution_clock::now();
        double build_sec = std::chrono::duration<double>(t_build_end - t_build_start).count();
        std::cout << "[Init] Scenario built in " << std::fixed << std::setprecision(2) << build_sec << " s.\n";

        if (!err.accumulated_errors.empty()) {
            std::cerr << "[Warning] Scenario load reported issues:\n" << err.accumulated_errors << "\n";
        }

        std::cout << "[Init] Writing cached scenario to " << opts.scenario_file << " ...\n";
        sys::write_scenario_file(*state, native_scenario_path, 1);
        state->on_scenario_load();
    } else {
        std::cout << "[Init] Cached scenario loaded successfully.\n";
    }

    state->fill_unsaved_data();
    state->current_scene.game_in_progress = true;
    state->game_seed = 808080;

    std::cout << "\n[World Statistics]\n";
    std::cout << "  Provinces:  " << state->world.province_size() << "\n";
    std::cout << "  Nations:    " << state->world.nation_size() << "\n";
    std::cout << "  Markets:    " << state->world.market_size() << "\n";
    std::cout << "  Commodities:" << state->world.commodity_size() << "\n";
    std::cout << "  Total POPs: " << state->world.pop_size() << "\n\n";

    // Setup CSV logging
    std::ofstream csv(opts.csv_file);
    if (!csv.is_open()) {
        std::cerr << "[Error] Failed to open CSV output file: " << opts.csv_file << "\n";
        return 1;
    }

    csv << "Day,Date,Year,Month,DayOfMonth,Tick_ms,Total_Money,Treasuries,Banks,Investment_Pools,"
        << "Factories,Pops_Savings,Artisans,RGO_Banks,Markets,Global_GDP,"
        << "Money_Delta,Minted_Accum,Accounting_Error_Pct\n";

    MonetarySnapshot initial_snap = take_snapshot(*state);
    std::cout << "[Baseline 1836.1.1] Initial Global Money: £ " 
              << std::fixed << std::setprecision(1) << initial_snap.total << "\n";
    std::cout << "  - Treasuries:       £ " << initial_snap.treasury << "\n";
    std::cout << "  - POP Savings:      £ " << initial_snap.pops << "\n";
    std::cout << "  - Factory Banks:    £ " << initial_snap.factory << "\n";
    std::cout << "  - Bank Deposits:    £ " << initial_snap.bank << "\n";
    std::cout << "  - Investment Pools: £ " << initial_snap.investment_pool << "\n";
    std::cout << "  - Global GDP:       £ " << initial_snap.global_gdp << "\n\n";

    std::cout << "----------------------------------------------------------------------------------------------------\n";
    std::cout << " Day   | Date       | Tick (ms) | Global Money (£) | Global GDP (£)   | Money Delta (£) | Acc. Error\n";
    std::cout << "----------------------------------------------------------------------------------------------------\n";

    std::vector<double> tick_times;
    tick_times.reserve(opts.days);

    double minted_accumulator = 0.0;
    auto sim_start_time = std::chrono::high_resolution_clock::now();

    for (uint32_t day = 1; day <= opts.days; ++day) {
        auto t0 = std::chrono::high_resolution_clock::now();

        // Run single daily simulation tick
        state->single_game_tick();

        // Track endogenous bullion monetization (M0 injection from gold/silver RGOs)
        double daily_minted = 0.0;
        state->world.for_each_commodity([&](dcon::commodity_id c) {
            if (state->world.commodity_get_rgo_amount(c) < 0.f || !state->world.commodity_get_money_rgo(c)) return;
            state->world.for_each_province([&](auto province) {
                auto controller = state->world.province_get_nation_from_province_control(province);
                if (!controller) return;
                auto produced = state->world.province_get_rgo_output(province, c);
                daily_minted += produced * state->defines.gold_to_cash_rate * state->world.commodity_get_cost(c);
            });
        });
        minted_accumulator += daily_minted;

        auto t1 = std::chrono::high_resolution_clock::now();
        double tick_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        tick_times.push_back(tick_ms);

        // Record metrics periodically or at end
        if (day % opts.check_interval == 0 || day == opts.days) {
            auto ymd = state->current_date.to_ymd(state->start_date);
            MonetarySnapshot cur = take_snapshot(*state);

            double delta_m = cur.total - initial_snap.total;
            double diff = std::abs(delta_m - minted_accumulator);
            double error_pct = (cur.total > 0.0f) ? (diff / cur.total) * 100.0 : 0.0;

            // Format date string
            char date_str[16];
            std::snprintf(date_str, sizeof(date_str), "%04d.%02d.%02d", ymd.year, ymd.month, ymd.day);

            // Print console row
            std::cout << std::setw(6) << day << " | "
                      << date_str << " | "
                      << std::setw(9) << std::fixed << std::setprecision(2) << tick_ms << " | "
                      << std::setw(16) << std::fixed << std::setprecision(1) << cur.total << " | "
                      << std::setw(16) << std::fixed << std::setprecision(1) << cur.global_gdp << " | "
                      << std::setw(15) << std::fixed << std::setprecision(1) << delta_m << " | "
                      << std::setw(8) << std::fixed << std::setprecision(4) << error_pct << "%\n";

            // Write CSV row
            csv << day << "," << date_str << "," << ymd.year << "," << ymd.month << "," << ymd.day << ","
                << tick_ms << "," << cur.total << "," << cur.treasury << "," << cur.bank << ","
                << cur.investment_pool << "," << cur.factory << "," << cur.pops << ","
                << cur.artisans << "," << cur.rgo << "," << cur.market << "," << cur.global_gdp << ","
                << delta_m << "," << minted_accumulator << "," << error_pct << "\n";
            csv.flush();
        }
    }

    auto sim_end_time = std::chrono::high_resolution_clock::now();
    double total_sim_sec = std::chrono::duration<double>(sim_end_time - sim_start_time).count();

    csv.close();

    std::cout << "----------------------------------------------------------------------------------------------------\n\n";

    // Statistical calculations
    double avg_tick = std::accumulate(tick_times.begin(), tick_times.end(), 0.0) / tick_times.size();
    double min_tick = *std::min_element(tick_times.begin(), tick_times.end());
    double max_tick = *std::max_element(tick_times.begin(), tick_times.end());
    double tps = static_cast<double>(opts.days) / total_sim_sec;

    std::cout << "[Performance Summary]\n";
    std::cout << "  Simulated Days:   " << opts.days << "\n";
    std::cout << "  Wall Clock Time:  " << std::fixed << std::setprecision(2) << total_sim_sec << " s\n";
    std::cout << "  Simulation Speed: " << std::fixed << std::setprecision(1) << tps << " ticks/sec\n";
    std::cout << "  Average Tick:     " << std::fixed << std::setprecision(2) << avg_tick << " ms\n";
    std::cout << "  Minimum Tick:     " << std::fixed << std::setprecision(2) << min_tick << " ms\n";
    std::cout << "  Maximum Tick:     " << std::fixed << std::setprecision(2) << max_tick << " ms\n\n";

    std::cout << "[Baseline Exported]\n";
    std::cout << "  CSV File: " << opts.csv_file << "\n";
    std::cout << "  Status:   SUCCESS\n\n";

    return 0;
}
