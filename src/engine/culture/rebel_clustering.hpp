#pragma once
// =============================================================================
// rebel_clustering.hpp
// Rebel Army State-Clustering Engine
// Merges scattered 1~2 brigade roaming rebel armies of the same rebel faction
// located within the same state instance into unified provincial corps.
// Slashes rebel pathfinding, AI evaluation, and unit sprite clutter on mobile.
// =============================================================================

#include "system_state_forward.hpp"
#include "dcon_generated_ids.hpp"
#include <cstdint>

namespace vic2_mobile::culture {

struct RebelClusterStats {
    uint32_t states_checked = 0;
    uint32_t rebel_armies_examined = 0;
    uint32_t armies_consolidated = 0;
    uint32_t regiments_transferred = 0;
};

// Consolidates rebel armies within a single state instance
RebelClusterStats cluster_state_rebel_armies(sys::state& state, dcon::state_instance_id s);

// Consolidates rebel armies across all state instances
RebelClusterStats cluster_all_rebel_armies(sys::state& state);

} // namespace vic2_mobile::culture
