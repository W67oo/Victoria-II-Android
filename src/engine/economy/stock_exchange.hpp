#pragma once
// =============================================================================
// src/engine/economy/stock_exchange.hpp
// Phase 4: Corporate Stock Exchange, Enterprise Valuation, Public Equity IPOs,
// Capitalist Portfolio Investment, Secondary Market Pricing & Credit Crunch.
// =============================================================================

#include "dcon_generated_ids.hpp"
#include "system_state_forward.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace economy::stock_market {

struct FactoryStockQuote {
    dcon::factory_id factory;
    float share_price = 10.0f;       // £ per share
    float market_cap = 10000.0f;     // Total enterprise value in £
    float pe_ratio = 10.0f;          // Price-to-Earnings ratio
    float dividend_yield = 0.05f;    // Annualized dividend yield [0.0, 1.0]
    float trailing_profit = 0.0f;    // 30-day cumulative net profit
    uint32_t total_shares = 1000;    // Standard 1,000 shares per factory
    float state_share_ratio = 0.0f;  // [0.0, 1.0] held by state
    bool is_public = false;          // True if listed on national exchange
    bool in_distress = false;        // True if factory cash <= 0 & losses
};

struct NationalMarketMetrics {
    float composite_index = 1000.0f; // Baseline 1000.0 pts
    float total_market_cap = 0.0f;   // Aggregate equity value in £
    float average_pe = 10.0f;
    float credit_crunch_severity = 0.0f; // [0.0, 1.0] (0 = normal, 1 = total credit freeze)
    uint32_t listed_companies = 0;
    uint32_t distressed_companies = 0;
    bool credit_crunch_active = false;
};

// Lifecycle
void initialize(sys::state& state);
void daily_stock_market_tick(sys::state& state);
void weekly_stock_market_settlement(sys::state& state);

// Queries
FactoryStockQuote get_factory_quote(sys::state const& state, dcon::factory_id f);
NationalMarketMetrics get_national_metrics(sys::state const& state, dcon::nation_id n);

// Equity transactions
bool conduct_ipo(sys::state& state, dcon::factory_id f);
bool delist_factory(sys::state& state, dcon::factory_id f);

// Credit crunch check
bool is_nation_in_credit_crunch(sys::state const& state, dcon::nation_id n);

// State persistence & serialization
size_t serialize_size();
uint8_t* serialize(uint8_t* ptr_out);
uint8_t const* deserialize(uint8_t const* ptr_in);

} // namespace economy::stock_market
