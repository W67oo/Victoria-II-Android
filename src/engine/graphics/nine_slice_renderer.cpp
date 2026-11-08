#include "nine_slice_renderer.hpp"
#include "system_state.hpp"
#include "stateless_render.hpp"
#include "stateless_render_constants.hpp"

namespace ui {

void render_nine_slice(sys::state& state,
                       const slice_margins& margins,
                       float x, float y, float width, float height,
                       GLuint texture_handle,
                       ogl::color_modification color_mod,
                       ui::rotation r,
                       bool flipped,
                       bool rtl) {
    if (width <= 0.0f || height <= 0.0f || texture_handle == 0) {
        return;
    }

    glBindVertexArray(state.open_gl.global_square_vao);
    ogl::bind_vertices_by_rotation(state, r, flipped, rtl);

    glUniform4f(state.open_gl.ui_shader_d_rect_uniform, x, y, width, height);

    // If symmetric, use border_size; if asymmetric, pass border_size = 0.0f and margins in subrect
    if (margins.left == margins.top && margins.left == margins.right && margins.left == margins.bottom) {
        glUniform1f(state.open_gl.ui_shader_border_size_uniform, margins.left);
        glUniform4f(state.open_gl.ui_shader_subrect_uniform, 0.0f, 0.0f, 0.0f, 0.0f);
    } else {
        glUniform1f(state.open_gl.ui_shader_border_size_uniform, 0.0f);
        glUniform4f(state.open_gl.ui_shader_subrect_uniform,
                    margins.left, margins.top, margins.right, margins.bottom);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_handle);

    GLuint subroutines[2] = {
        ogl::map_color_modification_to_index(color_mod),
        ogl::parameters::frame_stretch
    };
    glUniform2ui(state.open_gl.ui_shader_subroutines_index_uniform, subroutines[0], subroutines[1]);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void render_nine_slice(sys::state& state,
                       const slice_margins& margins,
                       float x, float y, float width, float height,
                       dcon::gfx_object_id gfx_id,
                       ogl::color_modification color_mod,
                       ui::rotation r,
                       bool flipped,
                       bool rtl) {
    if(!bool(gfx_id)) return;
    auto const& gfx_def = state.ui_defs.gfx[gfx_id];
    if(!gfx_def.primary_texture_handle) return;
    GLuint tex = ogl::get_texture_handle(state, gfx_def.primary_texture_handle, gfx_def.is_partially_transparent());
    render_nine_slice(state, margins, x, y, width, height, tex, color_mod, r, flipped, rtl);
}

} // namespace ui
