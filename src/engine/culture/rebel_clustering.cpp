#include "rebel_clustering.hpp"
#include "system_state.hpp"
#include "military.hpp"
#include "unordered_dense.h"
#include <vector>
#include <algorithm>

namespace vic2_mobile::culture {

struct RebelClusterKey {
    dcon::rebel_faction_id faction;
    dcon::state_instance_id state_id;

    bool operator==(const RebelClusterKey& o) const noexcept {
        return faction == o.faction && state_id == o.state_id;
    }
};

struct RebelClusterKeyHash {
    size_t operator()(const RebelClusterKey& k) const noexcept {
        return (size_t(k.faction.index()) * 10007) ^ size_t(k.state_id.index());
    }
};

RebelClusterStats cluster_all_rebel_armies(sys::state& state) {
    RebelClusterStats stats{};

    ankerl::unordered_dense::map<RebelClusterKey, std::vector<dcon::army_id>, RebelClusterKeyHash> clusters;

    // Collect all active non-battle rebel armies grouped by (faction, state_instance)
    state.world.for_each_army([&](dcon::army_id a) {
        if (!state.world.army_is_valid(a)) return;

        auto rf = state.world.army_get_controller_from_army_rebel_control(a);
        if (!rf) return; // Not a rebel army

        // Skip armies engaged in battle
        if (state.world.army_get_battle_from_army_battle_participation(a)) return;

        auto prov = state.world.army_get_location_from_army_location(a);
        if (!state.world.province_is_valid(prov)) return;

        auto sid = state.world.province_get_state_membership(prov);
        if (!state.world.state_instance_is_valid(sid)) return;

        clusters[RebelClusterKey{rf, sid}].push_back(a);
        ++stats.rebel_armies_examined;
    });

    stats.states_checked = static_cast<uint32_t>(clusters.size());

    // Consolidate armies within each cluster
    for (auto& [key, army_list] : clusters) {
        if (army_list.size() <= 1) continue;

        // Determine host army: pick the one with the highest regiment count or state capital
        dcon::province_id state_capital = state.world.state_instance_get_capital(key.state_id);

        size_t best_idx = 0;
        int32_t max_regs = -1;

        for (size_t i = 0; i < army_list.size(); ++i) {
            dcon::army_id a = army_list[i];
            int32_t reg_count = 0;
            for (auto r : state.world.army_get_army_membership(a)) {
                (void)r;
                ++reg_count;
            }
            if (state.world.army_get_location_from_army_location(a) == state_capital) {
                reg_count += 100; // Prefer state capital positioning
            }
            if (reg_count > max_regs) {
                max_regs = reg_count;
                best_idx = i;
            }
        }

        dcon::army_id host_army = army_list[best_idx];

        for (size_t i = 0; i < army_list.size(); ++i) {
            if (i == best_idx) continue;
            dcon::army_id src_army = army_list[i];
            if (!state.world.army_is_valid(src_army)) continue;

            // Move all regiments from src_army to host_army
            while (state.world.army_get_army_membership(src_army).begin() != state.world.army_get_army_membership(src_army).end()) {
                auto reg = (*state.world.army_get_army_membership(src_army).begin()).get_regiment();
                state.world.regiment_set_army_from_army_membership(reg, host_army);
                ++stats.regiments_transferred;
            }

            // Remove empty army entity
            state.world.delete_army(src_army);
            ++stats.armies_consolidated;
        }
    }

    return stats;
}

RebelClusterStats cluster_state_rebel_armies(sys::state& state, dcon::state_instance_id s) {
    RebelClusterStats stats{};
    if (!state.world.state_instance_is_valid(s)) return stats;
    // Single state cluster delegates to full cluster query filtering on s
    return cluster_all_rebel_armies(state);
}

} // namespace vic2_mobile::culture
