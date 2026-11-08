// =============================================================================
// src/engine/economy/central_bank.cpp
// Phase 3 (3.4): Central Banking, Currency Standards, Floating Exchange Rates
// and Sovereign Credit Ratings for Victoria 2 / Project Alice.
// =============================================================================

#include "central_bank.hpp"
#include "system_state.hpp"
#include "economy_stats.hpp"
#include "economy_production.hpp"
#include "nations.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstring>

namespace economy {

static std::vector<central_bank_account> s_central_banks;

static float compute_nation_price_level(sys::state const& state, dcon::nation_id n) {
    float total_weighted_price = 0.0f;
    float total_base_cost = 0.0f;
    uint32_t count = 0;

    auto cap = state.world.nation_get_capital(n);
    if (!cap) return 1.0f;

    auto sid = state.world.province_get_state_membership(cap);
    auto mid = state.world.state_instance_get_market_from_local_market(sid);
    if (!mid) return 1.0f;

    state.world.for_each_commodity([&](dcon::commodity_id c) {
        float cost = state.world.commodity_get_cost(c);
        if (cost <= 0.001f) return;
        float p = state.world.market_get_price(mid, c);
        total_weighted_price += p;
        total_base_cost += cost;
        ++count;
    });

    if (total_base_cost <= 0.001f || count == 0) return 1.0f;
    return std::clamp(total_weighted_price / total_base_cost, 0.20f, 5.0f);
}

static std::string get_nation_tag_str(sys::state const& state, dcon::nation_id n) {
    if (!n) return "";
    auto ident = state.world.nation_get_identity_from_identity_holder(n);
    if (!ident) return "";
    uint32_t iint = state.world.national_identity_get_identifying_int(ident);
    return nations::int_to_tag(iint);
}

void initialize_central_banks(sys::state& state) {
    uint32_t nat_count = state.world.nation_size();
    s_central_banks.assign(nat_count, central_bank_account{});

    for (uint32_t i = 0; i < nat_count; ++i) {
        dcon::nation_id n{dcon::nation_id::value_base_t(i)};
        if (!nations::exists(state, n)) continue;

        auto& cb = s_central_banks[i];
        bool civ = state.world.nation_get_is_civilized(n);
        int32_t rank = state.world.nation_get_rank(n);

        // Historical gold/silver/bimetallic defaults based on rank and civilization status
        if (rank <= 3) {
            // Premier Great Powers (e.g. UK, France)
            cb.standard = currency_standard::gold_standard;
            cb.credit_rating = sovereign_credit_rating::AAA;
            cb.base_interest_rate = 0.025f; // 2.5%
            cb.bullion_reserve = 50000.0f;
        } else if (rank <= 8) {
            // Secondary Great Powers (e.g. Prussia, USA, Russia, Austria)
            cb.standard = currency_standard::bimetallic_standard;
            cb.credit_rating = sovereign_credit_rating::AA;
            cb.base_interest_rate = 0.035f; // 3.5%
            cb.bullion_reserve = 25000.0f;
        } else if (civ) {
            // Civilized secondary powers
            cb.standard = currency_standard::silver_standard;
            cb.credit_rating = sovereign_credit_rating::A;
            cb.base_interest_rate = 0.045f; // 4.5%
            cb.bullion_reserve = 10000.0f;
        } else {
            // Uncivilized nations
            cb.standard = currency_standard::silver_standard;
            cb.credit_rating = sovereign_credit_rating::BB;
            cb.base_interest_rate = 0.065f; // 6.5%
            cb.bullion_reserve = 3000.0f;
        }

        cb.bond_yield = cb.base_interest_rate + get_credit_spread(cb.credit_rating);
        cb.price_level_index = 1.0f;
        cb.inflation_rate = 0.0f;
        cb.exchange_rate_vs_anchor = 1.0f;
        cb.ppp_anchor = 1.0f;
    }
}

void update_central_banks_weekly(sys::state& state) {
    uint32_t nat_count = state.world.nation_size();
    if (s_central_banks.size() < nat_count) {
        initialize_central_banks(state);
    }

    for (uint32_t i = 0; i < nat_count; ++i) {
        dcon::nation_id n{dcon::nation_id::value_base_t(i)};
        if (!nations::exists(state, n)) continue;

        auto& cb = s_central_banks[i];

        // 1. Price Index & Inflation
        float current_price_level = compute_nation_price_level(state, n);
        float old_price_level = (cb.price_level_index > 0.001f) ? cb.price_level_index : 1.0f;
        cb.inflation_rate = std::clamp((current_price_level - old_price_level) / old_price_level * 52.0f, -0.20f, 0.50f);
        cb.price_level_index = current_price_level;

        // 2. Sovereign Debt & Credit Rating
        float debt = state.world.nation_get_local_loan(n);
        float gdp = std::max(10.0f, economy::gdp::value_nation(state, n));
        int32_t total_banks = 0;
        for (auto p : state.world.nation_get_province_ownership(n)) {
            total_banks += state.world.province_get_building_level(p.get_province(), uint8_t(economy::province_building_type::bank));
        }
        // Provincial bank infrastructure bolsters sovereign fiscal resilience
        float bank_shield = 1.0f / (1.0f + std::min(0.25f, total_banks * 0.01f));
        float debt_to_gdp = (debt / gdp) * bank_shield;

        if (debt_to_gdp < 0.20f) {
            cb.credit_rating = sovereign_credit_rating::AAA;
        } else if (debt_to_gdp < 0.40f) {
            cb.credit_rating = sovereign_credit_rating::AA;
        } else if (debt_to_gdp < 0.60f) {
            cb.credit_rating = sovereign_credit_rating::A;
        } else if (debt_to_gdp < 0.85f) {
            cb.credit_rating = sovereign_credit_rating::BBB;
        } else if (debt_to_gdp < 1.15f) {
            cb.credit_rating = sovereign_credit_rating::BB;
        } else if (debt_to_gdp < 1.50f) {
            cb.credit_rating = sovereign_credit_rating::B;
        } else if (debt_to_gdp < 2.00f) {
            cb.credit_rating = sovereign_credit_rating::CCC;
        } else {
            cb.credit_rating = sovereign_credit_rating::C;
        }

        // 3. Central Bank Policy Rate (Taylor Reaction Rule for AI nations)
        // If human player, preserve player's manual policy decision unless AI controls it
        if (n != state.local_player_nation) {
            if (cb.inflation_rate > 0.03f) {
                cb.base_interest_rate = std::min(0.20f, cb.base_interest_rate + 0.0025f);
            } else if (cb.inflation_rate < -0.01f) {
                cb.base_interest_rate = std::max(0.015f, cb.base_interest_rate - 0.0025f);
            }
        }

        cb.bond_yield = cb.base_interest_rate + get_credit_spread(cb.credit_rating);

        // 4. Purchasing Power Parity (PPP Anchor) & Exchange Rate
        cb.ppp_anchor = std::clamp(current_price_level, 0.10f, 10.0f);

        switch (cb.standard) {
            case currency_standard::gold_standard:
                // Gold export/import points tightly restrict FX within +-2%
                cb.exchange_rate_vs_anchor = std::clamp(cb.ppp_anchor, 0.98f, 1.02f);
                // If bullion reserve completely exhausted under deficits, break gold peg
                if (cb.bullion_reserve < 500.0f) {
                    cb.standard = currency_standard::fiat_standard; // Forced off gold standard
                }
                break;
            case currency_standard::silver_standard:
                // Silver standard fluctuates with silver/gold market relative value
                cb.exchange_rate_vs_anchor = std::clamp(cb.ppp_anchor * 1.10f, 0.80f, 1.50f);
                break;
            case currency_standard::bimetallic_standard:
                cb.exchange_rate_vs_anchor = std::clamp(cb.ppp_anchor, 0.95f, 1.15f);
                break;
            case currency_standard::fiat_standard:
                // Freely floating paper currency
                cb.exchange_rate_vs_anchor = std::clamp(cb.ppp_anchor, 0.05f, 20.0f);
                break;
        }
    }
}

central_bank_account const& get_central_bank(sys::state const& state, dcon::nation_id n) {
    static central_bank_account s_fallback{};
    if (n.index() < s_central_banks.size()) {
        return s_central_banks[n.index()];
    }
    return s_fallback;
}

central_bank_account& get_central_bank_mut(sys::state& state, dcon::nation_id n) {
    if (s_central_banks.size() < state.world.nation_size()) {
        initialize_central_banks(state);
    }
    return s_central_banks[n.index()];
}

float get_exchange_rate(sys::state const& state, dcon::nation_id n) {
    return get_central_bank(state, n).exchange_rate_vs_anchor;
}

float get_bond_yield(sys::state const& state, dcon::nation_id n) {
    return get_central_bank(state, n).bond_yield;
}

float get_credit_spread(sovereign_credit_rating rating) {
    switch (rating) {
        case sovereign_credit_rating::AAA:     return 0.000f; // +0.0%
        case sovereign_credit_rating::AA:      return 0.007f; // +0.7%
        case sovereign_credit_rating::A:       return 0.015f; // +1.5%
        case sovereign_credit_rating::BBB:     return 0.027f; // +2.7%
        case sovereign_credit_rating::BB:      return 0.045f; // +4.5%
        case sovereign_credit_rating::B:       return 0.070f; // +7.0%
        case sovereign_credit_rating::CCC:     return 0.105f; // +10.5%
        case sovereign_credit_rating::C:       return 0.150f; // +15.0%
        case sovereign_credit_rating::Default: return 0.250f; // +25.0%
        default: return 0.030f;
    }
}

const char* get_rating_str(sovereign_credit_rating rating) {
    switch (rating) {
        case sovereign_credit_rating::AAA:     return "AAA";
        case sovereign_credit_rating::AA:      return "AA";
        case sovereign_credit_rating::A:       return "A";
        case sovereign_credit_rating::BBB:     return "BBB";
        case sovereign_credit_rating::BB:      return "BB";
        case sovereign_credit_rating::B:       return "B";
        case sovereign_credit_rating::CCC:     return "CCC";
        case sovereign_credit_rating::C:       return "C";
        case sovereign_credit_rating::Default: return "Default";
        default: return "NR";
    }
}

const char* get_currency_symbol(sys::state const& state, dcon::nation_id n) {
    if (!n) return "£";
    auto tag = get_nation_tag_str(state, n);
    // Well-known national currencies
    if (tag == "ENG") return "£"; // British Pound
    if (tag == "FRA") return "₣"; // French Franc
    if (tag == "USA") return "$"; // US Dollar
    if (tag == "PRU" || tag == "GER") return "Th"; // Prussian Thaler / German Mark
    if (tag == "RUS") return "₽"; // Russian Ruble
    if (tag == "AUS") return "fl"; // Austrian Gulden
    if (tag == "TUR") return "Lt"; // Ottoman Lira
    if (tag == "CHI") return "两"; // Qing Tael
    if (tag == "JAP") return "円"; // Japanese Yen
    if (tag == "SPA") return "₧"; // Spanish Peseta
    if (tag == "ITA") return "₤"; // Italian Lira
    if (tag == "SWE") return "kr"; // Swedish Krona
    if (tag == "NET") return "fl"; // Dutch Guilder
    return "£"; // Standard international clearing unit default
}

const char* get_currency_name(sys::state const& state, dcon::nation_id n) {
    if (!n) return "Pound";
    auto tag = get_nation_tag_str(state, n);
    if (tag == "ENG") return "Pound";
    if (tag == "FRA") return "Franc";
    if (tag == "USA") return "Dollar";
    if (tag == "PRU" || tag == "GER") return "Mark";
    if (tag == "RUS") return "Ruble";
    if (tag == "AUS") return "Gulden";
    if (tag == "TUR") return "Lira";
    if (tag == "CHI") return "Tael";
    if (tag == "JAP") return "Yen";
    if (tag == "SPA") return "Peseta";
    if (tag == "ITA") return "Lira";
    if (tag == "SWE") return "Krona";
    if (tag == "NET") return "Guilder";
    return "Currency";
}

size_t serialize_central_banks_size() {
    return sizeof(uint32_t) + s_central_banks.size() * sizeof(central_bank_account);
}

uint8_t* serialize_central_banks(uint8_t* ptr_out) {
    uint32_t count = static_cast<uint32_t>(s_central_banks.size());
    memcpy(ptr_out, &count, sizeof(uint32_t));
    ptr_out += sizeof(uint32_t);
    if (count > 0) {
        memcpy(ptr_out, s_central_banks.data(), count * sizeof(central_bank_account));
        ptr_out += count * sizeof(central_bank_account);
    }
    return ptr_out;
}

uint8_t const* deserialize_central_banks(uint8_t const* ptr_in) {
    uint32_t count = 0;
    memcpy(&count, ptr_in, sizeof(uint32_t));
    ptr_in += sizeof(uint32_t);
    s_central_banks.resize(count);
    if (count > 0) {
        memcpy(s_central_banks.data(), ptr_in, count * sizeof(central_bank_account));
        ptr_in += count * sizeof(central_bank_account);
    }
    return ptr_in;
}

} // namespace economy
