package com.vic2mobile.app

import android.content.Context
import android.content.res.AssetManager
import java.io.File
import java.io.FileNotFoundException
import java.io.FileOutputStream

/**
 * Shared bootstrap helper ensuring both LauncherActivity and Vic2GameActivity
 * (including direct restoration after Android background process death) always
 * have extracted Project Alice built-in assets and valid native runtime paths.
 */
object RuntimeBootstrap {

    private const val STAMP_FILE_NAME = ".alice_assets_v1"

    fun gameRootDir(context: Context): File {
        val extBase = context.getExternalFilesDir(null) ?: context.filesDir
        return File(extBase, "vic2_data").apply { mkdirs() }
    }

    fun aliceBuiltinDir(context: Context): File {
        return File(context.filesDir, "alice_builtin").apply { mkdirs() }
    }

    fun runtimeCacheDir(context: Context): File {
        val extBase = context.getExternalFilesDir(null) ?: context.filesDir
        return File(extBase, "runtime").apply { mkdirs() }
    }

    fun resolveActualVic2Root(base: File): File {
        if (File(base, "common/bookmarks.txt").exists() || File(base, "common").isDirectory) {
            return base
        }
        val subDirs = base.listFiles()?.filter { it.isDirectory } ?: emptyList()
        if (subDirs.size == 1 && File(subDirs[0], "common").isDirectory) {
            return subDirs[0]
        }
        for (sub in subDirs) {
            if (File(sub, "common/bookmarks.txt").exists()) return sub
        }
        return base
    }

    fun isVic2DataReady(context: Context): Boolean {
        val actual = resolveActualVic2Root(gameRootDir(context))
        return File(actual, "common/bookmarks.txt").exists() && File(actual, "map").isDirectory
    }

    @Synchronized
    fun ensureAliceBuiltinAssets(context: Context): File {
        val targetRoot = aliceBuiltinDir(context)
        val stampFile = File(targetRoot, STAMP_FILE_NAME)
        if (!stampFile.exists()) {
            copyAssetFolderRecursive(context.assets, "alice_builtin", targetRoot)
            stampFile.writeText("ok")
        }
        return targetRoot
    }

    fun syncRuntimePathsToNative(context: Context) {
        val resolvedGameRoot = resolveActualVic2Root(gameRootDir(context))
        val aliceRoot = aliceBuiltinDir(context)
        val cacheRoot = runtimeCacheDir(context)
        NativeBridge.nativeSetRuntimePaths(
            vic2AssetsRoot = resolvedGameRoot.absolutePath,
            aliceAssetsRoot = aliceRoot.absolutePath,
            cacheDir = cacheRoot.absolutePath,
            displayDensity = context.resources.displayMetrics.density
        )
    }

    private fun copyAssetFolderRecursive(am: AssetManager, assetPath: String, destDir: File) {
        val children = am.list(assetPath) ?: return
        if (children.isEmpty()) {
            destDir.parentFile?.mkdirs()
            try {
                am.open(assetPath).use { input ->
                    FileOutputStream(destDir).use { output ->
                        input.copyTo(output, bufferSize = 32 * 1024)
                    }
                }
            } catch (_: FileNotFoundException) {
                // Empty directory inside APK assets
                destDir.mkdirs()
            }
        } else {
            destDir.mkdirs()
            for (child in children) {
                val subAsset = if (assetPath.isEmpty()) child else "$assetPath/$child"
                copyAssetFolderRecursive(am, subAsset, File(destDir, child))
            }
        }
    }
}
