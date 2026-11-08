#pragma once
// =============================================================================
// src/engine/economy/central_bank.hpp
// Phase 3 (3.4): Central Banking, Currency Standards, Floating Exchange Rates
// and Sovereign Credit Ratings for Victoria 2 / Project Alice.
//
// Dual-Tier Architecture:
//   - Tier 1 (Engine Core): All physical ledgers, 11 pools and balance sheets
//     retain high-precision gold standard accounting (£).
//   - Tier 2 (National Central Banking & FX): Each sovereign state issues its
//     national currency with an independent currency standard, central bank
//     discount rate, bullion reserves, CPI price index, PPP exchange rate,
//     and sovereign credit rating.
// =============================================================================

#include "dcon_generated_ids.hpp"
#include "system_state_forward.hpp"
#include <cstdint>
#include <string_view>

namespace economy {

enum class currency_standard : uint8_t {
    gold_standard = 0,      // Classical Gold Standard (hard specie backing, strict parity +-2%)
    silver_standard,        // Silver Standard (East Asia, Latin America)
    bimetallic_standard,    // Bimetallic Standard (Latin Monetary Union, USA pre-1873)
    fiat_standard           // Paper / Inconvertible Fiat Standard (freely floating)
};

enum class sovereign_credit_rating : uint8_t {
    AAA = 0,    // Prime (e.g. Great Britain, France) - 2.5% yield (+0.0% spread)
    AA,         // High grade (e.g. USA, Prussia) - 3.2% yield (+0.7% spread)
    A,          // Upper medium grade - 4.0% yield (+1.5% spread)
    BBB,        // Lower medium grade (e.g. Russia, Austria) - 5.2% yield (+2.7% spread)
    BB,         // Speculative (e.g. Ottoman Empire, Spain) - 7.0% yield (+4.5% spread)
    B,          // Highly speculative - 9.5% yield (+7.0% spread)
    CCC,        // Substantial risk of default - 13.0% yield (+10.5% spread)
    C,          // Near default / sovereign debt restructuring - 18.0%+ yield (+15.0% spread)
    Default     // In default
};

struct central_bank_account {
    currency_standard standard = currency_standard::gold_standard;
    sovereign_credit_rating credit_rating = sovereign_credit_rating::A;
    float base_interest_rate = 0.05f;       // 5.0% baseline central bank discount rate
    float bullion_reserve = 10000.0f;       // Vault specie gold/silver reserve (£ equivalent)
    float price_level_index = 1.0f;         // CPI base (1.0 = 1836 baseline par)
    float inflation_rate = 0.0f;            // Annualized inflation rate (-0.05 = -5%, +0.10 = +10%)
    float exchange_rate_vs_anchor = 1.0f;   // Domestic currency per international clearing £ (1.0 = par)
    float bond_yield = 0.05f;               // Government borrowing yield: base_rate + credit_spread
    float weekly_bop_balance = 0.0f;        // Balance of Payments: Net exports + net income flows
    float ppp_anchor = 1.0f;                // Purchasing Power Parity fundamental anchor
};

// Subsystem lifecycle
void initialize_central_banks(sys::state& state);
void update_central_banks_weekly(sys::state& state);

// Accessors
central_bank_account const& get_central_bank(sys::state const& state, dcon::nation_id n);
central_bank_account& get_central_bank_mut(sys::state& state, dcon::nation_id n);
float get_exchange_rate(sys::state const& state, dcon::nation_id n);
float get_bond_yield(sys::state const& state, dcon::nation_id n);
float get_credit_spread(sovereign_credit_rating rating);
const char* get_rating_str(sovereign_credit_rating rating);
const char* get_currency_symbol(sys::state const& state, dcon::nation_id n);
const char* get_currency_name(sys::state const& state, dcon::nation_id n);

// State persistence & serialization
size_t serialize_central_banks_size();
uint8_t* serialize_central_banks(uint8_t* ptr_out);
uint8_t const* deserialize_central_banks(uint8_t const* ptr_in);

} // namespace economy
