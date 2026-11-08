#pragma once
// =============================================================================
// src/engine/economy/dual_ownership.hpp
// Phase 3 (3.5): Dual-Track Ownership Structure (State vs Private Capital),
// Factory Equity Shares, State Enterprise Profit Remittance, and Capitalist Dividends.
// =============================================================================

#include "dcon_generated_ids.hpp"
#include "system_state_forward.hpp"
#include <cstdint>

namespace economy::dual_ownership {

enum class nationalization_mode : uint8_t {
    compensated = 0,    // Buyout using state treasury into capitalist investment pool
    confiscation        // Expropriation without compensation (raises militancy)
};

// Subsystem lifecycle
void initialize(sys::state& state);
void reset_daily_stats();

// Factory state ownership share accessors (0.0 = 100% private, 1.0 = 100% state-owned)
float get_factory_state_ownership(sys::state const& state, dcon::factory_id f);
void set_factory_state_ownership(sys::state& state, dcon::factory_id f, float state_share);

// Profit split and tracking
void record_state_dividend(dcon::nation_id n, float amount);
float get_daily_state_dividends(dcon::nation_id n);

// Factory construction & expansion equity adjustments
void on_factory_created(sys::state& state, dcon::factory_id f, dcon::province_id p, bool is_pop_project);
void on_factory_expanded(sys::state& state, dcon::factory_id f, float old_size, float added_size, bool is_pop_project);

// Sovereign equity transactions
bool nationalize_factory(sys::state& state, dcon::factory_id f, nationalization_mode mode);
bool privatize_factory(sys::state& state, dcon::factory_id f);

// State persistence & serialization
size_t serialize_size();
uint8_t* serialize(uint8_t* ptr_out);
uint8_t const* deserialize(uint8_t const* ptr_in);

} // namespace economy::dual_ownership
