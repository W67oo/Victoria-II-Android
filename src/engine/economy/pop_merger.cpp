#include "pop_merger.hpp"
#include "system_state.hpp"
#include "demographics.hpp"
#include "unordered_dense.h"
#include <vector>
#include <algorithm>

namespace vic2_mobile::economy {

struct PopGroupKey {
    dcon::culture_id culture;
    dcon::religion_id religion;
    dcon::pop_type_id pop_type;

    bool operator==(const PopGroupKey& o) const noexcept {
        return culture == o.culture && religion == o.religion && pop_type == o.pop_type;
    }
};

struct PopGroupKeyHash {
    size_t operator()(const PopGroupKey& k) const noexcept {
        size_t h1 = size_t(k.culture.index());
        size_t h2 = size_t(k.religion.index());
        size_t h3 = size_t(k.pop_type.index());
        return (h1 * 31 + h2) * 31 + h3;
    }
};

PopMergeStats merge_province_micro_pops(sys::state& state, dcon::province_id prov, float size_threshold) {
    PopMergeStats stats{};
    if (!state.world.province_is_valid(prov)) return stats;

    stats.provinces_processed = 1;

    // Collect all valid pops in this province
    std::vector<dcon::pop_id> province_pops;
    for (auto pl : state.world.province_get_pop_location(prov)) {
        dcon::pop_id pid = pl.get_pop();
        if (state.world.pop_is_valid(pid)) {
            province_pops.push_back(pid);
        }
    }

    stats.pops_examined = static_cast<uint32_t>(province_pops.size());
    if (province_pops.size() <= 1) return stats;

    // Group pops by (culture, religion, pop_type)
    ankerl::unordered_dense::map<PopGroupKey, std::vector<dcon::pop_id>, PopGroupKeyHash> groups;
    for (auto p : province_pops) {
        PopGroupKey key{
            state.world.pop_get_culture(p),
            state.world.pop_get_religion(p),
            state.world.pop_get_poptype(p)
        };
        groups[key].push_back(p);
    }

    for (auto& [key, pop_list] : groups) {
        if (pop_list.size() <= 1) continue;

        // Verify if any POP is below the micro-POP threshold
        bool has_micro_pop = false;
        for (auto p : pop_list) {
            if (state.world.pop_get_size(p) < size_threshold) {
                has_micro_pop = true;
                break;
            }
        }
        if (!has_micro_pop) continue;

        // Choose the largest POP as the host consolidation target
        size_t best_idx = 0;
        float max_size = -1.0f;
        for (size_t i = 0; i < pop_list.size(); ++i) {
            float sz = state.world.pop_get_size(pop_list[i]);
            if (sz > max_size) {
                max_size = sz;
                best_idx = i;
            }
        }

        dcon::pop_id host_pop = pop_list[best_idx];
        float host_size = state.world.pop_get_size(host_pop);
        float host_lit = pop_demographics::get_literacy(state, host_pop);
        float host_mil = pop_demographics::get_militancy(state, host_pop);
        float host_con = pop_demographics::get_consciousness(state, host_pop);

        for (size_t i = 0; i < pop_list.size(); ++i) {
            if (i == best_idx) continue;
            dcon::pop_id src_pop = pop_list[i];
            if (!state.world.pop_is_valid(src_pop)) continue;

            float src_size = state.world.pop_get_size(src_pop);
            // Only merge if either source is small or host is already absorbing small fragments
            if (src_size >= size_threshold && host_size >= size_threshold) {
                continue;
            }

            float src_lit = pop_demographics::get_literacy(state, src_pop);
            float src_mil = pop_demographics::get_militancy(state, src_pop);
            float src_con = pop_demographics::get_consciousness(state, src_pop);

            float combined_size = host_size + src_size;
            if (combined_size > 0.0f) {
                host_lit = (host_size * host_lit + src_size * src_lit) / combined_size;
                host_mil = (host_size * host_mil + src_size * src_mil) / combined_size;
                host_con = (host_size * host_con + src_size * src_con) / combined_size;
                host_size = combined_size;
            }

            // Transfer military regiments belonging to source pop
            while (state.world.pop_get_regiment_source(src_pop).begin() != state.world.pop_get_regiment_source(src_pop).end()) {
                auto reg_src = *state.world.pop_get_regiment_source(src_pop).begin();
                reg_src.set_pop(host_pop);
                ++stats.regiments_transferred;
            }

            // Transfer land constructions belonging to source pop
            while (state.world.pop_get_province_land_construction(src_pop).begin() != state.world.pop_get_province_land_construction(src_pop).end()) {
                auto constr_src = *state.world.pop_get_province_land_construction(src_pop).begin();
                constr_src.set_pop(host_pop);
            }

            // Delete merged source POP
            state.world.delete_pop(src_pop);
            ++stats.pops_merged;
        }

        // Commit updated weighted properties to host POP
        state.world.pop_set_size(host_pop, host_size);
        state.world.pop_set_uliteracy(host_pop, pop_demographics::to_pu16(host_lit));
        pop_demographics::set_militancy(state, host_pop, host_mil);
        pop_demographics::set_consciousness(state, host_pop, host_con);
    }

    return stats;
}

PopMergeStats merge_all_micro_pops(sys::state& state, float size_threshold) {
    PopMergeStats total{};
    int32_t total_provs = state.province_definitions.first_sea_province.index();
    for (int32_t i = 0; i < total_provs; ++i) {
        dcon::province_id pid{ dcon::province_id::value_base_t(i) };
        if (state.world.province_is_valid(pid)) {
            auto s = merge_province_micro_pops(state, pid, size_threshold);
            total.provinces_processed += s.provinces_processed;
            total.pops_examined += s.pops_examined;
            total.pops_merged += s.pops_merged;
            total.regiments_transferred += s.regiments_transferred;
        }
    }
    return total;
}

PopMergeStats merge_staggered_micro_pops(sys::state& state, uint32_t day_index, uint32_t total_divisions, float size_threshold) {
    PopMergeStats total{};
    if (total_divisions == 0) total_divisions = 30;
    int32_t total_provs = state.province_definitions.first_sea_province.index();
    for (int32_t i = 0; i < total_provs; ++i) {
        if (static_cast<uint32_t>(i % total_divisions) == (day_index % total_divisions)) {
            dcon::province_id pid{ dcon::province_id::value_base_t(i) };
            if (state.world.province_is_valid(pid)) {
                auto s = merge_province_micro_pops(state, pid, size_threshold);
                total.provinces_processed += s.provinces_processed;
                total.pops_examined += s.pops_examined;
                total.pops_merged += s.pops_merged;
                total.regiments_transferred += s.regiments_transferred;
            }
        }
    }
    return total;
}

} // namespace vic2_mobile::economy
