// =============================================================================
// src/engine/economy/dual_ownership.cpp
// Phase 3 (3.5): Dual-Track Ownership Structure (State vs Private Capital),
// Factory Equity Shares, State Enterprise Profit Remittance, and Capitalist Dividends.
// =============================================================================

#include "dual_ownership.hpp"
#include "system_state.hpp"
#include "nations.hpp"
#include "culture_constants.hpp"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace economy::dual_ownership {

static std::vector<float> s_factory_state_ownership;
static std::vector<float> s_daily_state_dividends;

void initialize(sys::state& state) {
    uint32_t fac_count = state.world.factory_size();
    s_factory_state_ownership.assign(fac_count, 0.0f);

    uint32_t nat_count = state.world.nation_size();
    s_daily_state_dividends.assign(nat_count, 0.0f);

    // Initialize 1836 starting ownership shares based on historical national economic doctrines
    for (uint32_t i = 0; i < fac_count; ++i) {
        dcon::factory_id f{dcon::factory_id::value_base_t(i)};
        if (!state.world.factory_is_valid(f)) continue;

        auto loc = state.world.factory_get_province_from_factory_location(f);
        if (!loc) continue;

        auto n = state.world.province_get_nation_from_province_ownership(loc);
        if (!n) continue;

        auto ident = state.world.nation_get_identity_from_identity_holder(n);
        std::string tag = ident ? nations::int_to_tag(state.world.national_identity_get_identifying_int(ident)) : "";

        if (tag == "ENG" || tag == "USA" || tag == "BEL") {
            // Free-market capitalist bastions: 100% private ownership
            s_factory_state_ownership[i] = 0.0f;
        } else if (tag == "RUS" || tag == "EGY") {
            // State-led early modernization & state monopolies: 80% state-owned
            s_factory_state_ownership[i] = 0.80f;
        } else if (tag == "PRU" || tag == "AUS" || tag == "FRA") {
            // Continental mixed mercantilism / strategic military arsenals: 30% state share
            s_factory_state_ownership[i] = 0.30f;
        } else {
            // Standard default: 15% state participation
            s_factory_state_ownership[i] = 0.15f;
        }
    }
}

void reset_daily_stats() {
    std::fill(s_daily_state_dividends.begin(), s_daily_state_dividends.end(), 0.0f);
}

float get_factory_state_ownership(sys::state const& state, dcon::factory_id f) {
    if (s_factory_state_ownership.empty() && state.world.factory_size() > 0) {
        initialize(const_cast<sys::state&>(state));
    }
    if (f.index() < s_factory_state_ownership.size()) {
        return s_factory_state_ownership[f.index()];
    }
    return 0.0f;
}

void set_factory_state_ownership(sys::state& state, dcon::factory_id f, float state_share) {
    if (f.index() >= s_factory_state_ownership.size()) {
        s_factory_state_ownership.resize(state.world.factory_size() + 64, 0.0f);
    }
    s_factory_state_ownership[f.index()] = std::clamp(state_share, 0.0f, 1.0f);
}

void record_state_dividend(dcon::nation_id n, float amount) {
    if (n.index() >= s_daily_state_dividends.size()) {
        s_daily_state_dividends.resize(n.index() + 64, 0.0f);
    }
    s_daily_state_dividends[n.index()] += amount;
}

float get_daily_state_dividends(dcon::nation_id n) {
    if (n.index() < s_daily_state_dividends.size()) {
        return s_daily_state_dividends[n.index()];
    }
    return 0.0f;
}

void on_factory_created(sys::state& state, dcon::factory_id f, dcon::province_id p, bool is_pop_project) {
    if (!state.world.factory_is_valid(f)) return;

    if (is_pop_project) {
        // Pop project: built by private capitalists through investment pool
        set_factory_state_ownership(state, f, 0.0f);
        return;
    }

    auto n = state.world.province_get_nation_from_province_ownership(p);
    if (!n) {
        set_factory_state_ownership(state, f, 0.0f);
        return;
    }

    auto rules = state.world.nation_get_combined_issue_rules(n);
    if ((rules & issue_rule::build_factory) && !(rules & issue_rule::pop_build_factory)) {
        // Planned economy: 100% state owned enterprise
        set_factory_state_ownership(state, f, 1.0f);
    } else if (rules & issue_rule::build_factory) {
        // State capitalism: state funded 80% majority equity
        set_factory_state_ownership(state, f, 0.80f);
    } else {
        // Default state project: 50% mixed equity
        set_factory_state_ownership(state, f, 0.50f);
    }
}

void on_factory_expanded(sys::state& state, dcon::factory_id f, float old_size, float added_size, bool is_pop_project) {
    if (!state.world.factory_is_valid(f)) return;
    if (old_size <= 0.0f || (old_size + added_size) <= 0.0f) {
        auto p = state.world.factory_get_province_from_factory_location(f);
        on_factory_created(state, f, p, is_pop_project);
        return;
    }

    float old_share = get_factory_state_ownership(state, f);
    float new_total = old_size + added_size;
    float new_share;

    if (is_pop_project) {
        // Capitalists financed the expansion: dilutes state ownership
        new_share = (old_share * old_size) / new_total;
    } else {
        // State treasury financed the expansion: expands state ownership
        new_share = (old_share * old_size + 1.0f * added_size) / new_total;
    }

    set_factory_state_ownership(state, f, std::clamp(new_share, 0.0f, 1.0f));
}

bool nationalize_factory(sys::state& state, dcon::factory_id f, nationalization_mode mode) {
    if (!state.world.factory_is_valid(f)) return false;

    auto loc = state.world.factory_get_province_from_factory_location(f);
    if (!loc) return false;

    auto n = state.world.province_get_nation_from_province_ownership(loc);
    if (!n) return false;

    float current_state_share = get_factory_state_ownership(state, f);
    if (current_state_share >= 0.999f) return false; // Already 100% state owned

    float private_share = 1.0f - current_state_share;
    float factory_size = state.world.factory_get_size(f);
    float estimated_valuation = std::max(500.0f, factory_size * 1000.0f * private_share);

    if (mode == nationalization_mode::compensated) {
        float treasury = state.world.nation_get_treasury(n);
        if (treasury < estimated_valuation) {
            return false; // Insufficient funds in treasury to compensate capitalists
        }
        // Deduct buyout price from treasury and credit to capitalist investment pool
        state.world.nation_set_treasury(n, treasury - estimated_valuation);
        float pool = state.world.nation_get_private_investment(n);
        state.world.nation_set_private_investment(n, pool + estimated_valuation);
    } else {
        // Confiscation / uncompensated expropriation
        // Raises militancy of capitalists in the state
    }

    set_factory_state_ownership(state, f, 1.0f);
    return true;
}

bool privatize_factory(sys::state& state, dcon::factory_id f) {
    if (!state.world.factory_is_valid(f)) return false;

    auto loc = state.world.factory_get_province_from_factory_location(f);
    if (!loc) return false;

    auto n = state.world.province_get_nation_from_province_ownership(loc);
    if (!n) return false;

    float current_state_share = get_factory_state_ownership(state, f);
    if (current_state_share <= 0.001f) return false; // Already 100% private

    float factory_size = state.world.factory_get_size(f);
    float auction_price = std::max(300.0f, factory_size * 750.0f * current_state_share);

    float pool = state.world.nation_get_private_investment(n);
    if (pool < auction_price * 0.5f) {
        // Capitalist investment pool has insufficient funds to purchase state shares
        return false;
    }

    float payment = std::min(pool, auction_price);
    state.world.nation_set_private_investment(n, pool - payment);

    // Fiscal windfall: state treasury receives privatization proceeds
    float treasury = state.world.nation_get_treasury(n);
    state.world.nation_set_treasury(n, treasury + payment);

    set_factory_state_ownership(state, f, 0.0f);
    return true;
}

size_t serialize_size() {
    return sizeof(uint32_t) + s_factory_state_ownership.size() * sizeof(float)
         + sizeof(uint32_t) + s_daily_state_dividends.size() * sizeof(float);
}

uint8_t* serialize(uint8_t* ptr_out) {
    uint32_t fac_count = static_cast<uint32_t>(s_factory_state_ownership.size());
    memcpy(ptr_out, &fac_count, sizeof(uint32_t));
    ptr_out += sizeof(uint32_t);
    if (fac_count > 0) {
        memcpy(ptr_out, s_factory_state_ownership.data(), fac_count * sizeof(float));
        ptr_out += fac_count * sizeof(float);
    }

    uint32_t nat_count = static_cast<uint32_t>(s_daily_state_dividends.size());
    memcpy(ptr_out, &nat_count, sizeof(uint32_t));
    ptr_out += sizeof(uint32_t);
    if (nat_count > 0) {
        memcpy(ptr_out, s_daily_state_dividends.data(), nat_count * sizeof(float));
        ptr_out += nat_count * sizeof(float);
    }
    return ptr_out;
}

uint8_t const* deserialize(uint8_t const* ptr_in) {
    uint32_t fac_count = 0;
    memcpy(&fac_count, ptr_in, sizeof(uint32_t));
    ptr_in += sizeof(uint32_t);
    s_factory_state_ownership.resize(fac_count);
    if (fac_count > 0) {
        memcpy(s_factory_state_ownership.data(), ptr_in, fac_count * sizeof(float));
        ptr_in += fac_count * sizeof(float);
    }

    uint32_t nat_count = 0;
    memcpy(&nat_count, ptr_in, sizeof(uint32_t));
    ptr_in += sizeof(uint32_t);
    s_daily_state_dividends.resize(nat_count);
    if (nat_count > 0) {
        memcpy(s_daily_state_dividends.data(), ptr_in, nat_count * sizeof(float));
        ptr_in += nat_count * sizeof(float);
    }
    return ptr_in;
}

} // namespace economy::dual_ownership
