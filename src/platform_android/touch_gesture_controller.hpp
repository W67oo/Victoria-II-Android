#pragma once
// =============================================================================
// touch_gesture_controller.hpp
// Mobile Multi-Touch State Machine & On-Demand Frame Throttler (15/60 FPS)
// Implements:
//   - Tap (<250ms, <12dp): Select / Execute (or Inspect-Only if [?] active)
//   - Hold (>=250ms UI / >=300ms Map with Army): Pin Tooltip / Army March (RClick)
//   - 1-Finger Pan: Camera translation (never triggers accidental army move)
//   - 2-Finger Pinch: Smooth zoom mapped to wheel delta
//   - 2-Finger Top-Left Tap: Toggle built-in developer console
// =============================================================================

#include <cmath>
#include <cstdint>

namespace vic2_mobile::input {

enum class TouchActionType : uint8_t {
    None = 0,
    PrimaryTap,          // Mapped to LButton Down+Up (or Tooltip Inspect when inspector_mode_ == true)
    LongPressMarchOrTip, // >= 300ms on map (RButton army march) or >= 250ms on UI (Pin Tooltip)
    PanMove,             // Camera pan delta
    PinchZoom,           // Smooth zoom wheel delta
    ToggleDebugConsole   // 2-finger tap on top-left flag region
};

struct TouchCommand {
    TouchActionType type = TouchActionType::None;
    int32_t x = 0;
    int32_t y = 0;
    float delta_x = 0.0f;
    float delta_y = 0.0f;
    float zoom_delta = 0.0f;
};

struct SafeAreaInsets {
    int32_t left = 0;
    int32_t top = 0;
    int32_t right = 0;
    int32_t bottom = 0;
};

class TouchGestureController {
public:
    static constexpr uint64_t kTooltipHoldMs   = 250;
    static constexpr uint64_t kArmyMarchHoldMs = 300;
    static constexpr uint64_t kIdleThrottleMs  = 1500;

    void set_viewport(int32_t width, int32_t height, float dpi_scale, SafeAreaInsets insets) noexcept {
        width_ = width;
        height_ = height;
        dpi_scale_ = (dpi_scale > 0.1f) ? dpi_scale : 1.0f;
        safe_insets_ = insets;
    }

    void set_inspector_mode(bool enabled) noexcept { inspector_mode_ = enabled; }
    bool inspector_mode() const noexcept { return inspector_mode_; }

    void set_box_select_mode(bool enabled) noexcept { box_select_mode_ = enabled; }
    bool box_select_mode() const noexcept { return box_select_mode_; }

    TouchCommand on_finger_down(int64_t finger_id, float norm_x, float norm_y, uint64_t now_ms) noexcept {
        last_input_ms_ = now_ms;
        int32_t px = static_cast<int32_t>(std::lround(norm_x * width_));
        int32_t py = static_cast<int32_t>(std::lround(norm_y * height_));

        if (active_fingers_ == 0) {
            primary_id_ = finger_id;
            start_x_ = cur_x_ = px;
            start_y_ = cur_y_ = py;
            down_ms_ = now_ms;
            dragging_ = false;
            long_press_fired_ = false;
            active_fingers_ = 1;
        } else if (active_fingers_ == 1 && finger_id != primary_id_) {
            secondary_id_ = finger_id;
            sec_x_ = px;
            sec_y_ = py;
            last_pinch_dist_ = distance(cur_x_, cur_y_, sec_x_, sec_y_);
            active_fingers_ = 2;
            dragging_ = false;

            // Developer Console shortcut: 2-finger tap in top-left flag zone
            const int32_t flag_zone_w = static_cast<int32_t>(140.0f * dpi_scale_) + safe_insets_.left;
            const int32_t flag_zone_h = static_cast<int32_t>(80.0f * dpi_scale_) + safe_insets_.top;
            if (cur_x_ <= flag_zone_w && cur_y_ <= flag_zone_h &&
                sec_x_ <= flag_zone_w * 2 && sec_y_ <= flag_zone_h * 2) {
                long_press_fired_ = true;
                return TouchCommand{TouchActionType::ToggleDebugConsole, cur_x_, cur_y_, 0.f, 0.f, 0.f};
            }
        }
        return {};
    }

    TouchCommand on_finger_motion(int64_t finger_id, float norm_x, float norm_y, uint64_t now_ms) noexcept {
        last_input_ms_ = now_ms;
        int32_t px = static_cast<int32_t>(std::lround(norm_x * width_));
        int32_t py = static_cast<int32_t>(std::lround(norm_y * height_));

        if (active_fingers_ == 1 && finger_id == primary_id_) {
            float dx = static_cast<float>(px - cur_x_);
            float dy = static_cast<float>(py - cur_y_);
            float total_move = distance(start_x_, start_y_, px, py);
            const float slop_px = 12.0f * dpi_scale_;

            cur_x_ = px;
            cur_y_ = py;

            if (!dragging_ && total_move >= slop_px) {
                dragging_ = true;
            }
            if (dragging_ && !long_press_fired_) {
                return TouchCommand{TouchActionType::PanMove, cur_x_, cur_y_, dx, dy, 0.0f};
            }
        } else if (active_fingers_ >= 2) {
            if (finger_id == primary_id_) {
                cur_x_ = px;
                cur_y_ = py;
            } else if (finger_id == secondary_id_) {
                sec_x_ = px;
                sec_y_ = py;
            }
            float new_dist = distance(cur_x_, cur_y_, sec_x_, sec_y_);
            float diff = new_dist - last_pinch_dist_;
            if (std::fabs(diff) > 2.0f * dpi_scale_) {
                last_pinch_dist_ = new_dist;
                int32_t mid_x = (cur_x_ + sec_x_) / 2;
                int32_t mid_y = (cur_y_ + sec_y_) / 2;
                float wheel = diff / (60.0f * dpi_scale_);
                return TouchCommand{TouchActionType::PinchZoom, mid_x, mid_y, 0.0f, 0.0f, wheel};
            }
        }
        return {};
    }

    TouchCommand on_finger_up(int64_t finger_id, float norm_x, float norm_y, uint64_t now_ms) noexcept {
        last_input_ms_ = now_ms;
        int32_t px = static_cast<int32_t>(std::lround(norm_x * width_));
        int32_t py = static_cast<int32_t>(std::lround(norm_y * height_));

        TouchCommand cmd{};
        if (active_fingers_ == 1 && finger_id == primary_id_) {
            if (!dragging_ && !long_press_fired_ && (now_ms - down_ms_) < kArmyMarchHoldMs) {
                cmd = TouchCommand{TouchActionType::PrimaryTap, px, py, 0.0f, 0.0f, 0.0f};
            }
            active_fingers_ = 0;
            dragging_ = false;
        } else if (active_fingers_ >= 2) {
            if (finger_id == primary_id_) {
                primary_id_ = secondary_id_;
                cur_x_ = sec_x_;
                cur_y_ = sec_y_;
            }
            active_fingers_ = 1;
            long_press_fired_ = true; // Suppress accidental tap after releasing pinch
        }
        return cmd;
    }

    // Polled once per frame to trigger 0.25s/0.30s hold without waiting for finger movement
    TouchCommand poll_hold(uint64_t now_ms, bool over_ui_element) noexcept {
        if (active_fingers_ == 1 && !dragging_ && !long_press_fired_) {
            uint64_t threshold = over_ui_element ? kTooltipHoldMs : kArmyMarchHoldMs;
            if (now_ms - down_ms_ >= threshold) {
                long_press_fired_ = true;
                return TouchCommand{TouchActionType::LongPressMarchOrTip, cur_x_, cur_y_, 0.0f, 0.0f, 0.0f};
            }
        }
        return {};
    }

    // Dynamic FPS cap: 15 FPS (66ms frame budget) when paused & untouched for >1.5s; 60 FPS (16ms) otherwise
    uint32_t target_frame_delay_ms(uint64_t now_ms, bool simulation_paused) const noexcept {
        if (simulation_paused && active_fingers_ == 0 && (now_ms - last_input_ms_ > kIdleThrottleMs)) {
            return 66; // 15 FPS power-saving mode
        }
        return 0; // VSync 60 FPS
    }

private:
    static float distance(int32_t x0, int32_t y0, int32_t x1, int32_t y1) noexcept {
        float dx = static_cast<float>(x1 - x0);
        float dy = static_cast<float>(y1 - y0);
        return std::sqrt(dx * dx + dy * dy);
    }

    int32_t width_ = 2400;
    int32_t height_ = 1080;
    float dpi_scale_ = 2.5f;
    SafeAreaInsets safe_insets_{};

    int32_t active_fingers_ = 0;
    int64_t primary_id_ = -1;
    int64_t secondary_id_ = -1;
    int32_t start_x_ = 0, start_y_ = 0;
    int32_t cur_x_ = 0, cur_y_ = 0;
    int32_t sec_x_ = 0, sec_y_ = 0;
    float last_pinch_dist_ = 0.0f;

    uint64_t down_ms_ = 0;
    uint64_t last_input_ms_ = 0;
    bool dragging_ = false;
    bool long_press_fired_ = false;
    bool inspector_mode_ = false;
    bool box_select_mode_ = false;
};

} // namespace vic2_mobile::input
