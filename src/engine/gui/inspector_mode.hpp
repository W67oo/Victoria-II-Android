#pragma once
// =============================================================================
// inspector_mode.hpp
// Global Inspector Mode [?] State Machine
// When enabled:
//   - Tapping any UI element, number, or modifier inspects & pins its tooltip
//   - Prevents accidental execution of the underlying button action
// =============================================================================

#include <atomic>

namespace vic2_mobile::ui {

class InspectorMode {
public:
    static InspectorMode& get() noexcept {
        static InspectorMode instance;
        return instance;
    }

    bool is_active() const noexcept {
        return active_.load(std::memory_order_relaxed);
    }

    void set_active(bool enabled) noexcept {
        active_.store(enabled, std::memory_order_relaxed);
    }

    bool toggle() noexcept {
        bool current = active_.load(std::memory_order_relaxed);
        bool next = !current;
        active_.store(next, std::memory_order_relaxed);
        return next;
    }

private:
    InspectorMode() = default;
    std::atomic<bool> active_{false};
};

} // namespace vic2_mobile::ui
