package com.vic2mobile.app

import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.WindowManager
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import java.io.BufferedInputStream
import java.io.File
import java.io.FileOutputStream
import java.util.concurrent.Executors
import java.util.zip.ZipInputStream

/**
 * BYOA (Bring Your Own Assets) Bootstrap & Safe-Area Detector
 * Extracts built-in Project Alice shaders/UI/Lua assets, imports user's Victoria II (.zip)
 * via Android SAF without storage permission bloat, and launches Vic2GameActivity.
 */
class LauncherActivity : AppCompatActivity() {

    private lateinit var statusText: TextView
    private lateinit var progressBar: ProgressBar
    private lateinit var importButton: Button
    private lateinit var launchButton: Button

    private lateinit var gameRoot: File

    private val ioExecutor = Executors.newSingleThreadExecutor()

    private val pickZipLauncher = registerForActivityResult(
        ActivityResultContracts.OpenDocument()
    ) { uri: Uri? ->
        if (uri != null) {
            extractVic2ZipAsync(uri)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        WindowCompat.setDecorFitsSystemWindows(window, false)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            window.attributes.layoutInDisplayCutoutMode =
                WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES
        }
        WindowInsetsControllerCompat(window, window.decorView).apply {
            hide(WindowInsetsCompat.Type.systemBars())
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        gameRoot = RuntimeBootstrap.gameRootDir(this)
        RuntimeBootstrap.syncRuntimePathsToNative(this)

        val rootLayout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
            setBackgroundColor(0xFF1E1812.toInt()) // Classic Victoria II dark mahogany
            setPadding(72, 48, 72, 48)
        }

        val titleText = TextView(this).apply {
            text = "VICTORIA II MOBILE · PROJECT ALICE"
            setTextColor(0xFFD4AF37.toInt()) // Victorian brass gold
            textSize = 20f
            gravity = Gravity.CENTER
            setPadding(0, 0, 0, 16)
        }

        statusText = TextView(this).apply {
            setTextColor(0xFFE6D5B8.toInt())
            textSize = 14f
            gravity = Gravity.CENTER
            setPadding(0, 0, 0, 24)
        }

        progressBar = ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal).apply {
            isIndeterminate = false
            max = 100
            progress = 0
            visibility = android.view.View.GONE
            setPadding(0, 0, 0, 24)
        }

        val buttonRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER
        }

        importButton = Button(this).apply {
            text = "导入原版 Victoria II (.zip)"
            setOnClickListener {
                pickZipLauncher.launch(arrayOf("application/zip", "application/x-zip-compressed", "application/octet-stream"))
            }
        }

        launchButton = Button(this).apply {
            text = "启动引擎进入游戏"
            isEnabled = false
            setOnClickListener {
                RuntimeBootstrap.syncRuntimePathsToNative(this@LauncherActivity)
                startActivity(Intent(this@LauncherActivity, Vic2GameActivity::class.java))
            }
        }

        buttonRow.addView(importButton)
        buttonRow.addView(launchButton)

        rootLayout.addView(titleText)
        rootLayout.addView(statusText)
        rootLayout.addView(progressBar)
        rootLayout.addView(buttonRow)
        setContentView(rootLayout)

        ViewCompat.setOnApplyWindowInsetsListener(rootLayout) { _, windowInsets ->
            val insets = windowInsets.getInsets(
                WindowInsetsCompat.Type.displayCutout() or WindowInsetsCompat.Type.systemGestures()
            )
            NativeBridge.nativeSetSafeAreaInsets(insets.left, insets.top, insets.right, insets.bottom)
            windowInsets
        }

        prepareBuiltinAssetsAndCheckGameData()
    }

    private fun prepareBuiltinAssetsAndCheckGameData() {
        statusText.text = "正在校验引擎内置着色器 (GLSL ES 3.20) 与 UI 布局资源..."
        ioExecutor.execute {
            try {
                RuntimeBootstrap.ensureAliceBuiltinAssets(this)
                val ready = RuntimeBootstrap.isVic2DataReady(this)
                runOnUiThread {
                    RuntimeBootstrap.syncRuntimePathsToNative(this)
                    launchButton.isEnabled = ready
                    statusText.text = if (ready) {
                        "已检测到完整 Victoria II 游戏数据 (${RuntimeBootstrap.resolveActualVic2Root(gameRoot).absolutePath})\n点击「启动引擎进入游戏」即可加载或首次流式生成 1836 剧本缓存 (.bin)"
                    } else {
                        "未检测到原版游戏目录。\n请点击「导入原版 Victoria II (.zip)」选择正版 Victoria II 打包文件（含 common / map / history / gfx）"
                    }
                }
            } catch (e: Exception) {
                runOnUiThread {
                    statusText.text = "内置资源初始化异常: ${e.message}"
                }
            }
        }
    }

    private fun extractVic2ZipAsync(uri: Uri) {
        importButton.isEnabled = false
        launchButton.isEnabled = false
        progressBar.visibility = android.view.View.VISIBLE
        progressBar.isIndeterminate = true
        statusText.text = "正在流式解压原版 Victoria II 资源包至应用沙盒..."

        ioExecutor.execute {
            var count = 0
            try {
                val canonicalRoot = gameRoot.canonicalPath + File.separator
                contentResolver.openInputStream(uri)?.use { rawIn ->
                    ZipInputStream(BufferedInputStream(rawIn, 64 * 1024)).use { zis ->
                        val buffer = ByteArray(64 * 1024)
                        var entry = zis.nextEntry
                        while (entry != null) {
                            val cleanName = entry.name.replace('\\', '/')
                            val outFile = File(gameRoot, cleanName)
                            if (!outFile.canonicalPath.startsWith(canonicalRoot)) {
                                throw SecurityException("Blocked zip-slip entry: ${entry.name}")
                            }
                            if (entry.isDirectory) {
                                outFile.mkdirs()
                            } else {
                                outFile.parentFile?.mkdirs()
                                FileOutputStream(outFile).use { fos ->
                                    var len = zis.read(buffer)
                                    while (len > 0) {
                                        fos.write(buffer, 0, len)
                                        len = zis.read(buffer)
                                    }
                                }
                                count++
                                if (count % 200 == 0) {
                                    val c = count
                                    runOnUiThread {
                                        statusText.text = "正在流式解压 Victoria II 资源文件... 已解压 $c 个文件"
                                    }
                                }
                            }
                            zis.closeEntry()
                            entry = zis.nextEntry
                        }
                    }
                }

                val ready = RuntimeBootstrap.isVic2DataReady(this)
                runOnUiThread {
                    progressBar.visibility = android.view.View.GONE
                    importButton.isEnabled = true
                    RuntimeBootstrap.syncRuntimePathsToNative(this)
                    launchButton.isEnabled = ready
                    statusText.text = if (ready) {
                        "解压完成（共 $count 个文件）！校验通过。\n点击「启动引擎进入游戏」开始运行。"
                    } else {
                        "解压完成（$count 个文件），但未找到 common/bookmarks.txt，请确认压缩包为完整 Victoria II 目录。"
                    }
                }
            } catch (e: Exception) {
                runOnUiThread {
                    progressBar.visibility = android.view.View.GONE
                    importButton.isEnabled = true
                    statusText.text = "解压失败: ${e.message}"
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        ioExecutor.shutdownNow()
    }
}
