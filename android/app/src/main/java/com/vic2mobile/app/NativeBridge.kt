package com.vic2mobile.app

object NativeBridge {
    init {
        System.loadLibrary("c++_shared")
        System.loadLibrary("tbb")
        System.loadLibrary("SDL2")
        System.loadLibrary("vic2mobile")
    }

    @JvmStatic
    external fun nativeSetRuntimePaths(
        vic2AssetsRoot: String,
        aliceAssetsRoot: String,
        cacheDir: String,
        displayDensity: Float
    )

    @JvmStatic
    external fun nativeSetSafeAreaInsets(
        left: Int,
        top: Int,
        right: Int,
        bottom: Int
    )

    @JvmStatic
    external fun nativeGetScenarioStage(): Int

    @JvmStatic
    external fun nativeSetInspectorMode(enabled: Boolean)
}
