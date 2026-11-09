// opengl_wrapper_android.cpp
//
// Equivalente ao opengl_wrapper_nix.cpp. La a criacao do contexto acontece no
// glfwCreateWindow e aqui so se torna o contexto atual; no Android o contexto
// e a superficie EGL sao criados pelo window_android.cpp (APP_CMD_INIT_WINDOW),
// entao aqui tambem so garantimos que ele esta ativo nesta thread.
// Nao ha GLEW: as funcoes GLES vem direto da libGLESv3 (ver android_gl/glew.h).

#include "opengl_wrapper.hpp"
#include "system_state.hpp"

#include <EGL/egl.h>

namespace ogl {

void create_opengl_context(sys::state& state) {
	assert(state.win_ptr && state.win_ptr->egl_context != EGL_NO_CONTEXT);
	auto& win = *state.win_ptr;

	if(!eglMakeCurrent(win.egl_display, win.egl_surface, win.egl_surface, win.egl_context)) {
		window::emit_error_message("eglMakeCurrent falhou ao iniciar o OpenGL ES", true);
	}
	eglSwapInterval(win.egl_display, 1); // Vsync

	ALICE_LOGI("OpenGL ES Context bound: vendor=%s, renderer=%s, version=%s",
	           glGetString(GL_VENDOR) ? (const char*)glGetString(GL_VENDOR) : "unknown",
	           glGetString(GL_RENDERER) ? (const char*)glGetString(GL_RENDERER) : "unknown",
	           glGetString(GL_VERSION) ? (const char*)glGetString(GL_VERSION) : "unknown");
}

// o contexto EGL e destruido pelo window_android.cpp quando o app fecha
void shutdown_opengl(sys::state& state) { }

} // namespace ogl
