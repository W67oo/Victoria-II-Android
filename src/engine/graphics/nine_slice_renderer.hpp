#pragma once

// =============================================================================
// nine_slice_renderer.hpp
// Victoria 2 Android Mobile High-Performance 9-Slice Adaptive Frame Renderer.
// Provides 3x3 grid stretching with fixed corners and scalable edges/center
// for Victorian brass frames, parchment dialogs, drawers, and mobile tooltips.
// =============================================================================

#include "opengl_wrapper.hpp"
#include "constants_ui.hpp"
#include <glm/glm.hpp>

namespace ui {

struct slice_margins {
    float left = 16.0f;
    float top = 16.0f;
    float right = 16.0f;
    float bottom = 16.0f;

    constexpr slice_margins() noexcept = default;
    constexpr slice_margins(float uniform_margin) noexcept
        : left(uniform_margin), top(uniform_margin), right(uniform_margin), bottom(uniform_margin) {}
    constexpr slice_margins(float horiz, float vert) noexcept
        : left(horiz), top(vert), right(horiz), bottom(vert) {}
    constexpr slice_margins(float l, float t, float r, float b) noexcept
        : left(l), top(t), right(r), bottom(b) {}
};

// Common UI border presets matching Victoria 2 textures and mobile controls
namespace slice_presets {
    // Standard parchment dialog panel (tiles_dialog.tga)
    inline constexpr slice_margins parchment_panel{ 16.0f, 16.0f, 16.0f, 16.0f };

    // Ornate Victorian brass frame with 28px top header banner and 20px brass rails
    inline constexpr slice_margins brass_frame{ 20.0f, 28.0f, 20.0f, 20.0f };

    // Mobile slide-out drawer (left/right thin borders, clean top handle)
    inline constexpr slice_margins drawer_panel{ 12.0f, 20.0f, 12.0f, 16.0f };

    // Floating touch tooltip bubble
    inline constexpr slice_margins tooltip_bubble{ 10.0f, 10.0f, 10.0f, 10.0f };

    // Compact single-line mobile topbar capsule
    inline constexpr slice_margins topbar_capsule{ 8.0f, 8.0f, 8.0f, 8.0f };
}

// Renders an arbitrary textured rectangle using 9-slice mapping.
// Corners are preserved at 1:1 pixel scale, while borders and center are stretched.
void render_nine_slice(sys::state const& state,
                       const slice_margins& margins,
                       float x, float y, float width, float height,
                       GLuint texture_handle,
                       ogl::color_modification color_mod = ogl::color_modification::none,
                       ui::rotation r = ui::rotation::upright,
                       bool flipped = false,
                       bool rtl = false);

} // namespace ui
