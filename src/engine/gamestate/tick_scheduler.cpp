#include "tick_scheduler.hpp"
#include "system_state.hpp"
#include "pop_merger.hpp"
#include "rebel_clustering.hpp"

namespace vic2_mobile::scheduler {

static SchedulerStats g_stats{};

SchedulerStats& get_stats() noexcept {
    return g_stats;
}

void execute_daily_tier1(sys::state& /*state*/) {
    ++g_stats.total_daily_ticks;
}

void execute_weekly_tier2(sys::state& /*state*/, uint32_t /*day_of_week*/) {
    ++g_stats.weekly_ticks_run;
}

void execute_monthly_tier3(sys::state& state, uint32_t day_of_month) {
    ++g_stats.monthly_ticks_run;

    // Monthly micro-POP merger:
    // Staggered 1/30th per day to smooth out CPU spikes across all 30 days of the month!
    auto merge_res = vic2_mobile::economy::merge_staggered_micro_pops(state, day_of_month, 30, 500.0f);
    g_stats.micro_pops_merged += merge_res.pops_merged;

    // Rebel clustering: runs on day 15 and day 30 (twice a month) or on revolt pulses
    if (day_of_month == 15 || day_of_month == 30) {
        auto rebel_res = vic2_mobile::culture::cluster_all_rebel_armies(state);
        g_stats.rebel_armies_clustered += rebel_res.armies_consolidated;
    }
}

void execute_tiered_tick(sys::state& state) {
    execute_daily_tier1(state);

    auto ymd = state.current_date.to_ymd(state.start_date);
    uint32_t day_of_week = static_cast<uint32_t>(state.current_date.to_raw_value() % 7);
    execute_weekly_tier2(state, day_of_week);

    uint32_t day_of_month = static_cast<uint32_t>(ymd.day);
    execute_monthly_tier3(state, day_of_month);
}

} // namespace vic2_mobile::scheduler
