package com.vic2mobile.app

import android.os.Build
import android.os.Bundle
import android.view.WindowManager
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import org.libsdl.app.SDLActivity

/**
 * Fullscreen 20:9 Landscape OpenGL ES 3.2 Game Activity powered by SDL2 + Project Alice.
 * Forwards display cutout & gesture bar WindowInsets to C++20 UI layout safe-area.
 */
class Vic2GameActivity : SDLActivity() {

    override fun getLibraries(): Array<String> {
        return arrayOf("c++_shared", "tbb", "SDL2", "vic2mobile")
    }

    override fun getMainSharedObject(): String {
        return "libvic2mobile.so"
    }

    override fun getMainFunction(): String {
        return "SDL_main"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        // Ensure built-in shaders/Lua/GUI assets and runtime paths are ready BEFORE SDL_main starts,
        // even if Android restored Vic2GameActivity directly after background process death.
        RuntimeBootstrap.ensureAliceBuiltinAssets(this)
        RuntimeBootstrap.syncRuntimePathsToNative(this)

        super.onCreate(savedInstanceState)

        WindowCompat.setDecorFitsSystemWindows(window, false)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            window.attributes.layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        }
        WindowInsetsControllerCompat(window, window.decorView).apply {
            hide(WindowInsetsCompat.Type.systemBars())
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        ViewCompat.setOnApplyWindowInsetsListener(window.decorView) { _, windowInsets ->
            val insets = windowInsets.getInsets(
                WindowInsetsCompat.Type.displayCutout() or WindowInsetsCompat.Type.systemGestures()
            )
            NativeBridge.nativeSetSafeAreaInsets(insets.left, insets.top, insets.right, insets.bottom)
            windowInsets
        }
    }
}
