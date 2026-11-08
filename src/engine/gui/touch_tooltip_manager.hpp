#pragma once
// =============================================================================
// touch_tooltip_manager.hpp
// Pinned Tooltip State Machine for Touch Interactions & Inspector Mode
// Features:
//   - Pinned state persistence across touch-up events
//   - Safe viewport clipping, margin clamping & smart flip (below -> above)
//   - Tap-outside dismissal filter to prevent accidental button activation
// =============================================================================

#include <cstdint>
#include <algorithm>

namespace vic2_mobile::ui {

struct PinnedTooltipState {
    bool is_pinned = false;
    int32_t anchor_x = 0;
    int32_t anchor_y = 0;
    int32_t anchor_w = 0;
    int32_t anchor_h = 0;
    int32_t tooltip_x = 0;
    int32_t tooltip_y = 0;
    int32_t tooltip_w = 0;
    int32_t tooltip_h = 0;
    uint64_t pin_timestamp_ms = 0;
};

class TouchTooltipManager {
public:
    static TouchTooltipManager& get() noexcept {
        static TouchTooltipManager instance;
        return instance;
    }

    bool is_pinned() const noexcept {
        return state_.is_pinned;
    }

    const PinnedTooltipState& state() const noexcept {
        return state_;
    }

    // Calculates flipped / clamped position and pins the active tooltip
    void pin(int32_t anchor_x, int32_t anchor_y, int32_t anchor_w, int32_t anchor_h,
             int32_t tooltip_w, int32_t tooltip_h,
             int32_t screen_w, int32_t screen_h,
             int32_t safe_left, int32_t safe_top, int32_t safe_right, int32_t safe_bottom,
             uint64_t now_ms) noexcept {
        state_.is_pinned = true;
        state_.anchor_x = anchor_x;
        state_.anchor_y = anchor_y;
        state_.anchor_w = anchor_w;
        state_.anchor_h = anchor_h;
        state_.tooltip_w = tooltip_w;
        state_.tooltip_h = tooltip_h;
        state_.pin_timestamp_ms = now_ms;

        const int32_t margin = 12;
        const int32_t min_x = safe_left + margin;
        const int32_t max_x = std::max(min_x, screen_w - safe_right - tooltip_w - margin);
        const int32_t min_y = safe_top + margin;
        const int32_t max_y = std::max(min_y, screen_h - safe_bottom - tooltip_h - margin);

        // Horizontal: center over anchor, clamped to safe area
        int32_t ideal_x = anchor_x + (anchor_w / 2) - (tooltip_w / 2);
        state_.tooltip_x = std::clamp(ideal_x, min_x, max_x);

        // Vertical: preferred below anchor; flip above if overflowing bottom
        int32_t ideal_below_y = anchor_y + anchor_h + margin;
        if (ideal_below_y + tooltip_h <= screen_h - safe_bottom - margin) {
            state_.tooltip_y = ideal_below_y;
        } else {
            // Flip above anchor
            int32_t ideal_above_y = anchor_y - tooltip_h - margin;
            if (ideal_above_y >= min_y) {
                state_.tooltip_y = ideal_above_y;
            } else {
                // If neither fits cleanly, clamp inside viewport
                state_.tooltip_y = std::clamp(ideal_below_y, min_y, max_y);
            }
        }
    }

    void unpin() noexcept {
        state_.is_pinned = false;
    }

    // Returns true if a pinned tooltip was active and the tap was outside it (consuming the dismissal)
    bool handle_touch_tap(int32_t tap_x, int32_t tap_y, uint64_t now_ms) noexcept {
        if (!state_.is_pinned) {
            return false;
        }

        // Ignore instant release taps immediately after pinning (< 150ms)
        if (now_ms - state_.pin_timestamp_ms < 150) {
            return true;
        }

        bool inside_tooltip = (tap_x >= state_.tooltip_x && tap_x <= state_.tooltip_x + state_.tooltip_w &&
                               tap_y >= state_.tooltip_y && tap_y <= state_.tooltip_y + state_.tooltip_h);

        bool inside_anchor = (tap_x >= state_.anchor_x && tap_x <= state_.anchor_x + state_.anchor_w &&
                              tap_y >= state_.anchor_y && tap_y <= state_.anchor_y + state_.anchor_h);

        // Tap outside both tooltip and anchor dismisses the pinned tooltip
        if (!inside_tooltip && !inside_anchor) {
            unpin();
            return true; // Dismissed and consumed
        }

        return false;
    }

private:
    TouchTooltipManager() = default;
    PinnedTooltipState state_{};
};

} // namespace vic2_mobile::ui
