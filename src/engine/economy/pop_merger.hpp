#pragma once
// =============================================================================
// pop_merger.hpp
// Micro-POP Weighted Consolidation Engine (< 500 POPs)
// Merges small demographic fragments of identical (culture, religion, pop_type)
// within a province, reassigning military regiments & constructions, and
// recalculating population-weighted averages for literacy, militancy, consciousness.
// Reduces total POP array footprint by 30~60%, slashing mobile CPU demographic cost.
// =============================================================================

#include "system_state_forward.hpp"
#include "dcon_generated_ids.hpp"
#include <cstdint>

namespace vic2_mobile::economy {

struct PopMergeStats {
    uint32_t provinces_processed = 0;
    uint32_t pops_examined = 0;
    uint32_t pops_merged = 0;
    uint32_t regiments_transferred = 0;
};

// Merges micro-pops in a single province with size < size_threshold
PopMergeStats merge_province_micro_pops(sys::state& state, dcon::province_id prov, float size_threshold = 500.0f);

// Merges micro-pops across all land provinces
PopMergeStats merge_all_micro_pops(sys::state& state, float size_threshold = 500.0f);

// Staggered monthly partition: processes 1/N of provinces each day
PopMergeStats merge_staggered_micro_pops(sys::state& state, uint32_t day_index, uint32_t total_divisions = 30, float size_threshold = 500.0f);

} // namespace vic2_mobile::economy
