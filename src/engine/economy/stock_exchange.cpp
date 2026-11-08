// =============================================================================
// src/engine/economy/stock_exchange.cpp
// Phase 4: Corporate Stock Exchange, Enterprise Valuation, Public Equity IPOs,
// Capitalist Portfolio Investment, Secondary Market Pricing & Credit Crunch.
// =============================================================================

#include "stock_exchange.hpp"
#include "central_bank.hpp"
#include "dual_ownership.hpp"
#include "economy.hpp"
#include "system_state.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <cstring>

namespace economy::stock_market {

namespace {

struct InternalFactoryData {
    float trailing_30d_profit = 0.0f;
    float daily_profit_history[30]{};
    uint8_t history_idx = 0;
    float cash_reserves = 100.0f;
    float share_price = 10.0f;
    float market_cap = 10000.0f;
    float pe_ratio = 10.0f;
    float dividend_yield = 0.05f;
    bool is_public = false;
    bool in_distress = false;
};

struct InternalNationData {
    NationalMarketMetrics metrics{};
    float baseline_market_cap = 10000.0f;
    uint32_t consecutive_crunch_weeks = 0;
    float accumulated_pe = 0.0f;
};

std::unordered_map<uint32_t, InternalFactoryData> g_factory_data;
std::unordered_map<uint32_t, InternalNationData>  g_nation_data;

} // namespace

void initialize(sys::state& state) {
    g_factory_data.clear();
    g_nation_data.clear();

    // Initialize all existing factories with baseline market quotes
    state.world.for_each_factory([&](dcon::factory_id f) {
        InternalFactoryData d{};
        float size = std::max(1.0f, state.world.factory_get_size(f));
        d.cash_reserves = 100.0f * size;
        d.share_price = 10.0f * size;
        d.market_cap  = d.share_price * 1000.0f;
        d.pe_ratio    = 10.0f;
        d.dividend_yield = 0.05f;
        // Historical established factories in major powers start listed
        d.is_public = (size >= 1.0f);
        g_factory_data[f.index()] = d;
    });

    state.world.for_each_nation([&](dcon::nation_id n) {
        InternalNationData nd{};
        nd.metrics.composite_index = 1000.0f;
        nd.baseline_market_cap = 50000.0f;
        g_nation_data[n.index()] = nd;
    });
}

void daily_stock_market_tick(sys::state& state) {
    state.world.for_each_factory([&](dcon::factory_id f) {
        auto& d = g_factory_data[f.index()];
        float profit = state.world.factory_get_profit(f);

        // Update cash reserves: retain 20% of net profits or absorb 100% of losses
        if (profit > 0.0f) {
            d.cash_reserves = std::min(5000.0f, d.cash_reserves + profit * 0.20f);
        } else {
            d.cash_reserves = std::max(0.0f, d.cash_reserves + profit);
        }

        // Update 30-day circular buffer
        d.trailing_30d_profit -= d.daily_profit_history[d.history_idx];
        d.daily_profit_history[d.history_idx] = profit;
        d.trailing_30d_profit += profit;
        d.history_idx = static_cast<uint8_t>((d.history_idx + 1) % 30);

        // Financial distress check: depleted reserves and persistent negative net profit
        d.in_distress = (d.cash_reserves <= 10.0f && profit < -2.0f);
    });
}

void weekly_stock_market_settlement(sys::state& state) {
    // Reset national aggregators
    for (auto& [nid, nd] : g_nation_data) {
        nd.metrics.total_market_cap = 0.0f;
        nd.metrics.listed_companies = 0;
        nd.metrics.distressed_companies = 0;
        nd.accumulated_pe = 0.0f;
    }

    state.world.for_each_factory([&](dcon::factory_id f) {
        auto& d = g_factory_data[f.index()];
        auto p = state.world.factory_get_province_from_factory_location(f);
        auto n = state.world.province_get_nation_from_province_ownership(p);
        if (!n) return;

        float size = std::max(1.0f, state.world.factory_get_size(f));
        float replacement_val = 500.0f * size; // Base physical machinery replacement cost in £
        float annual_profit = std::max(0.0f, d.trailing_30d_profit * 12.0f);

        // Valuation driven by central bank benchmark interest rate
        float cb_rate = economy::get_central_bank(state, n).base_interest_rate;
        float risk_spread = d.in_distress ? 0.08f : 0.02f;
        float discount_rate = std::clamp(cb_rate + risk_spread, 0.04f, 0.25f);
        d.pe_ratio = std::clamp(1.0f / discount_rate, 4.0f, 25.0f);

        // Market cap calculation
        float earnings_value = annual_profit * d.pe_ratio;
        float intrinsic_val  = replacement_val + earnings_value;
        if (d.in_distress) {
            intrinsic_val *= 0.35f; // Severe fire-sale markdown under distress
        }
        d.market_cap = std::max(intrinsic_val, replacement_val * 0.20f);
        d.share_price = d.market_cap / 1000.0f;

        // Dividend yield
        float state_share = dual_ownership::get_factory_state_ownership(state, f);
        float private_annual_div = annual_profit * (1.0f - state_share);
        d.dividend_yield = (d.market_cap > 1.0f) ? std::clamp(private_annual_div / d.market_cap, 0.0f, 0.35f) : 0.0f;

        // Automatic IPO for mature, highly profitable unlisted factories
        if (!d.is_public && state.world.nation_get_is_civilized(n)) {
            if (d.trailing_30d_profit > 30.0f && d.cash_reserves > 150.0f) {
                d.is_public = true;
                // Capitalists subscribe to IPO shares using private_investment pool
                float inv_pool = state.world.nation_get_private_investment(n);
                float ipo_proceeds = std::min(d.market_cap * 0.25f, inv_pool * 0.15f);
                if (ipo_proceeds > 10.0f) {
                    state.world.nation_set_private_investment(n, inv_pool - ipo_proceeds);
                    d.cash_reserves += ipo_proceeds;
                }
            }
        }

        // Organic IPO: Profitable, cash-rich unlisted enterprises go public via equity underwriting
        if (!d.is_public && !d.in_distress && d.trailing_30d_profit > 15.0f && d.cash_reserves > 200.0f) {
            float inv_pool = state.world.nation_get_private_investment(n);
            if (inv_pool > 500.0f) {
                conduct_ipo(state, f);
            }
        }

        // Chronic insolvency delisting: If cash is zero and trailing losses severe, delist from exchange
        if (d.is_public && d.in_distress && d.cash_reserves <= 0.0f && d.trailing_30d_profit < -50.0f) {
            delist_factory(state, f);
        }

        // Secondary market portfolio investment by capitalists:
        // Capitalists invest surplus cash into healthy high-yield listed factories
        if (d.is_public && !d.in_distress && d.dividend_yield >= 0.05f) {
            float inv_pool = state.world.nation_get_private_investment(n);
            if (inv_pool > 200.0f) {
                float reinvestment = std::min(inv_pool * 0.02f, 25.0f);
                state.world.nation_set_private_investment(n, inv_pool - reinvestment);
                d.cash_reserves = std::min(10000.0f, d.cash_reserves + reinvestment);
                d.share_price *= 1.005f; // Small price boost from capital injection
            }
        }

        // Aggregate into national metrics
        if (d.is_public) {
            auto& nd = g_nation_data[n.index()];
            nd.metrics.total_market_cap += d.market_cap;
            nd.metrics.listed_companies++;
            nd.accumulated_pe += d.pe_ratio;
            if (d.in_distress) {
                nd.metrics.distressed_companies++;
            }
        }
    });

    // Update composite indices and evaluate credit crunch chain reaction
    for (auto& [nid_val, nd] : g_nation_data) {
        dcon::nation_id n{dcon::nation_id::value_base_t(nid_val)};
        if (nd.metrics.listed_companies == 0) continue;

        // Initialize baseline market cap on first populated week
        if (nd.baseline_market_cap <= 100.0f && nd.metrics.total_market_cap > 100.0f) {
            nd.baseline_market_cap = nd.metrics.total_market_cap;
        }

        // Composite stock index: 1000 baseline
        float ratio = (nd.baseline_market_cap > 1.0f)
            ? (nd.metrics.total_market_cap / nd.baseline_market_cap)
            : 1.0f;
        nd.metrics.composite_index = std::clamp(ratio * 1000.0f, 100.0f, 5000.0f);
        nd.metrics.average_pe = (nd.metrics.listed_companies > 0)
            ? std::clamp(nd.accumulated_pe / float(nd.metrics.listed_companies), 4.0f, 25.0f)
            : 10.0f;

        // Credit crunch condition: > 20% of publicly traded factories in financial distress
        float distress_rate = static_cast<float>(nd.metrics.distressed_companies) /
                              static_cast<float>(std::max(1u, nd.metrics.listed_companies));

        if (distress_rate >= 0.20f) {
            nd.metrics.credit_crunch_active = true;
            nd.metrics.credit_crunch_severity = std::clamp((distress_rate - 0.20f) / 0.30f, 0.0f, 1.0f);
            nd.consecutive_crunch_weeks++;

            // Chain reaction 1: Stock index crash penalty
            nd.metrics.composite_index *= (1.0f - 0.25f * nd.metrics.credit_crunch_severity);

            // Chain reaction 2: Capitalists freeze aggressive investments to hoard liquidity
            float cur_inv = state.world.nation_get_private_investment(n);
            if (cur_inv > 500.0f) {
                float liquidity_hoard = cur_inv * (0.02f * nd.metrics.credit_crunch_severity);
                state.world.nation_set_private_investment(n, cur_inv - liquidity_hoard);
            }
        } else {
            nd.metrics.credit_crunch_active = false;
            nd.metrics.credit_crunch_severity = 0.0f;
            nd.consecutive_crunch_weeks = 0;
        }
    }
}

FactoryStockQuote get_factory_quote(sys::state const& state, dcon::factory_id f) {
    if (g_factory_data.empty() && state.world.factory_size() > 0) {
        initialize(const_cast<sys::state&>(state));
    }
    FactoryStockQuote q{};
    q.factory = f;
    auto it = g_factory_data.find(f.index());
    if (it != g_factory_data.end()) {
        q.share_price      = it->second.share_price;
        q.market_cap       = it->second.market_cap;
        q.pe_ratio         = it->second.pe_ratio;
        q.dividend_yield   = it->second.dividend_yield;
        q.trailing_profit  = it->second.trailing_30d_profit;
        q.is_public        = it->second.is_public;
        q.in_distress      = it->second.in_distress;
    }
    q.state_share_ratio = dual_ownership::get_factory_state_ownership(state, f);
    return q;
}

NationalMarketMetrics get_national_metrics(sys::state const& state, dcon::nation_id n) {
    if (g_nation_data.empty() && state.world.nation_size() > 0) {
        initialize(const_cast<sys::state&>(state));
    }
    auto it = g_nation_data.find(n.index());
    if (it != g_nation_data.end()) {
        return it->second.metrics;
    }
    return NationalMarketMetrics{};
}

bool conduct_ipo(sys::state& state, dcon::factory_id f) {
    auto& d = g_factory_data[f.index()];
    if (d.is_public) return false;
    d.is_public = true;
    auto p = state.world.factory_get_province_from_factory_location(f);
    auto n = state.world.province_get_nation_from_province_ownership(p);
    if (n) {
        float inv = state.world.nation_get_private_investment(n);
        float proceeds = std::min(d.market_cap * 0.20f, inv * 0.10f);
        if (proceeds > 0.0f) {
            state.world.nation_set_private_investment(n, inv - proceeds);
            d.cash_reserves += proceeds;
        }
    }
    return true;
}

bool delist_factory(sys::state& /*state*/, dcon::factory_id f) {
    auto& d = g_factory_data[f.index()];
    if (!d.is_public) return false;
    d.is_public = false;
    return true;
}

bool is_nation_in_credit_crunch(sys::state const& /*state*/, dcon::nation_id n) {
    auto it = g_nation_data.find(n.index());
    if (it != g_nation_data.end()) {
        return it->second.metrics.credit_crunch_active;
    }
    return false;
}

size_t serialize_size() {
    size_t sz = sizeof(uint32_t); // factory count
    sz += g_factory_data.size() * (sizeof(uint32_t) + sizeof(InternalFactoryData));
    sz += sizeof(uint32_t); // nation count
    sz += g_nation_data.size() * (sizeof(uint32_t) + sizeof(InternalNationData));
    return sz;
}

uint8_t* serialize(uint8_t* ptr_out) {
    uint32_t f_count = static_cast<uint32_t>(g_factory_data.size());
    memcpy(ptr_out, &f_count, sizeof(uint32_t));
    ptr_out += sizeof(uint32_t);
    for (auto const& [k, v] : g_factory_data) {
        memcpy(ptr_out, &k, sizeof(uint32_t));
        ptr_out += sizeof(uint32_t);
        memcpy(ptr_out, &v, sizeof(InternalFactoryData));
        ptr_out += sizeof(InternalFactoryData);
    }

    uint32_t n_count = static_cast<uint32_t>(g_nation_data.size());
    memcpy(ptr_out, &n_count, sizeof(uint32_t));
    ptr_out += sizeof(uint32_t);
    for (auto const& [k, v] : g_nation_data) {
        memcpy(ptr_out, &k, sizeof(uint32_t));
        ptr_out += sizeof(uint32_t);
        memcpy(ptr_out, &v, sizeof(InternalNationData));
        ptr_out += sizeof(InternalNationData);
    }
    return ptr_out;
}

uint8_t const* deserialize(uint8_t const* ptr_in) {
    g_factory_data.clear();
    uint32_t f_count = 0;
    memcpy(&f_count, ptr_in, sizeof(uint32_t));
    ptr_in += sizeof(uint32_t);
    for (uint32_t i = 0; i < f_count; ++i) {
        uint32_t k = 0;
        memcpy(&k, ptr_in, sizeof(uint32_t));
        ptr_in += sizeof(uint32_t);
        InternalFactoryData v{};
        memcpy(&v, ptr_in, sizeof(InternalFactoryData));
        ptr_in += sizeof(InternalFactoryData);
        g_factory_data[k] = v;
    }

    g_nation_data.clear();
    uint32_t n_count = 0;
    memcpy(&n_count, ptr_in, sizeof(uint32_t));
    ptr_in += sizeof(uint32_t);
    for (uint32_t i = 0; i < n_count; ++i) {
        uint32_t k = 0;
        memcpy(&k, ptr_in, sizeof(uint32_t));
        ptr_in += sizeof(uint32_t);
        InternalNationData v{};
        memcpy(&v, ptr_in, sizeof(InternalNationData));
        ptr_in += sizeof(InternalNationData);
        g_nation_data[k] = v;
    }
    return ptr_in;
}

} // namespace economy::stock_market
