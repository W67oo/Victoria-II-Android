package com.vic2mobile.app;

import android.app.AlertDialog;
import android.app.NativeActivity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.provider.DocumentsContract;
import android.provider.Settings;
import android.util.Log;
import android.view.Gravity;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.Locale;

/**
 * Victoria II Mobile · NativeActivity (C++20 + GLES 3.2 + oneTBB).
 * Handles Scoped Storage permissions, SAF game directory picker,
 * non-blocking loading dialogs, and comprehensive on-device logging & crash reporting.
 */
public class AliceActivity extends NativeActivity {
	private static final String TAG = "Vic2Activity";
	private static final int REQUEST_PICK_FOLDER = 1;

	private static native void nativeOnGameFolderPicked(String path);
	public static native void nativeLog(int priority, String tag, String message);

	private static volatile Context appContext = null;
	private static boolean nativeLibsLoaded = false;

	static {
		installCrashHandler();
		loadNativeLibraries();
	}

	private static void loadNativeLibraries() {
		try {
			log(Log.INFO, TAG, "Loading libc++_shared.so...");
			System.loadLibrary("c++_shared");
		} catch (Throwable t) {
			log(Log.WARN, TAG, "c++_shared: " + t.getMessage());
		}

		try {
			log(Log.INFO, TAG, "Loading libtbb.so...");
			System.loadLibrary("tbb");
			log(Log.INFO, TAG, "libtbb.so loaded successfully");
		} catch (Throwable t) {
			recordFatalCrash("Failed to load libtbb.so", t);
			throw t;
		}

		try {
			log(Log.INFO, TAG, "Loading libvic2mobile.so...");
			System.loadLibrary("vic2mobile");
			nativeLibsLoaded = true;
			log(Log.INFO, TAG, "libvic2mobile.so loaded successfully with ANativeActivity_onCreate");
		} catch (Throwable t) {
			recordFatalCrash("Failed to load libvic2mobile.so", t);
			throw t;
		}
	}

	private static void installCrashHandler() {
		final Thread.UncaughtExceptionHandler defaultHandler = Thread.getDefaultUncaughtExceptionHandler();
		Thread.setDefaultUncaughtExceptionHandler((thread, throwable) -> {
			try {
				recordFatalCrash("Java Uncaught Exception in thread [" + thread.getName() + "]", throwable);
			} catch (Throwable ignored) {
			}
			if (defaultHandler != null) {
				defaultHandler.uncaughtException(thread, throwable);
			} else {
				System.exit(1);
			}
		});
	}

	public static void log(int priority, String tag, String message) {
		Log.println(priority, tag, message);
		appendToFileLog(priority, tag, message);
		if (nativeLibsLoaded) {
			try {
				nativeLog(priority, tag, message);
			} catch (Throwable ignored) {
			}
		}
	}

	private static synchronized void appendToFileLog(int priority, String tag, String message) {
		String timestamp = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US).format(new Date());
		String prioStr = priority == Log.ERROR ? "ERROR" : (priority == Log.WARN ? "WARN" : "INFO");
		String line = String.format("[%s] [%s/%s] %s\n", timestamp, tag, prioStr, message);

		if (appContext != null) {
			try {
				File extDir = appContext.getExternalFilesDir(null);
				if (extDir != null) {
					writeToFile(new File(extDir, "vic2_log.txt"), line, true);
				}
				File intDir = appContext.getFilesDir();
				if (intDir != null) {
					writeToFile(new File(intDir, "vic2_log.txt"), line, true);
				}
			} catch (Throwable ignored) {
			}
		}

		try {
			File pubLog = new File(Environment.getExternalStorageDirectory(), "vic2_log.txt");
			writeToFile(pubLog, line, true);
		} catch (Throwable ignored) {
		}
	}

	public static synchronized void recordFatalCrash(String title, Throwable throwable) {
		String timestamp = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US).format(new Date());
		StringWriter sw = new StringWriter();
		PrintWriter pw = new PrintWriter(sw);
		if (throwable != null) {
			throwable.printStackTrace(pw);
		}
		String stackTrace = sw.toString();

		long maxMemory = Runtime.getRuntime().maxMemory() / (1024 * 1024);
		long totalMemory = Runtime.getRuntime().totalMemory() / (1024 * 1024);
		long freeMemory = Runtime.getRuntime().freeMemory() / (1024 * 1024);

		StringBuilder sb = new StringBuilder();
		sb.append("=======================================================\n");
		sb.append("维多利亚 2 移动端 · 致命崩溃记录 (CRASH REPORT)\n");
		sb.append("时间 (Timestamp): ").append(timestamp).append("\n");
		sb.append("原因 (Reason): ").append(title).append("\n");
		sb.append("-------------------------------------------------------\n");
		sb.append("设备型号 (Device): ").append(Build.MANUFACTURER).append(" ").append(Build.MODEL)
		  .append(" (").append(Build.DEVICE).append(")\n");
		sb.append("安卓版本 (Android): ").append(Build.VERSION.RELEASE)
		  .append(" (SDK ").append(Build.VERSION.SDK_INT).append(")\n");
		sb.append("CPU 架构 (ABIs): ").append(Arrays.toString(Build.SUPPORTED_ABIS)).append("\n");
		sb.append("JVM 内存 (RAM MB): Total=").append(totalMemory).append(", Free=").append(freeMemory)
		  .append(", Max=").append(maxMemory).append("\n");
		sb.append("所有文件权限 (All-Files Access): ")
		  .append(Environment.isExternalStorageManager() ? "已授予 (Granted)" : "未授予 (Denied)").append("\n");
		sb.append("-------------------------------------------------------\n");
		sb.append("调用栈信息 (Stack Trace):\n");
		sb.append(stackTrace);
		sb.append("=======================================================\n\n");

		String report = sb.toString();
		Log.e(TAG, report);

		if (appContext != null) {
			try {
				File extDir = appContext.getExternalFilesDir(null);
				if (extDir != null) writeToFile(new File(extDir, "vic2_crash_log.txt"), report, true);
				File intDir = appContext.getFilesDir();
				if (intDir != null) writeToFile(new File(intDir, "vic2_crash_log.txt"), report, true);
			} catch (Throwable ignored) {}
		}
		try {
			File pubCrash = new File(Environment.getExternalStorageDirectory(), "vic2_crash_log.txt");
			writeToFile(pubCrash, report, true);
		} catch (Throwable ignored) {}

		appendToFileLog(Log.ERROR, "CRASH", report);
	}

	private static void writeToFile(File file, String content, boolean append) {
		try {
			FileOutputStream fos = new FileOutputStream(file, append);
			OutputStreamWriter osw = new OutputStreamWriter(fos, StandardCharsets.UTF_8);
			osw.write(content);
			osw.flush();
			fos.getFD().sync();
			osw.close();
		} catch (Throwable ignored) {
		}
	}

	private static String readFileToString(File file) {
		if (!file.exists() || !file.canRead()) return "";
		try (FileInputStream fis = new FileInputStream(file)) {
			byte[] data = new byte[(int) Math.min(file.length(), 65536)];
			int n = fis.read(data);
			return n > 0 ? new String(data, 0, n, StandardCharsets.UTF_8) : "";
		} catch (Throwable t) {
			return "";
		}
	}

	private boolean waitingForAllFilesAccess = false;
	private AlertDialog folderDialog = null;

	private final Handler uiHandler = new Handler(Looper.getMainLooper());
	private AlertDialog loadingDialog = null;
	private PopupWindow statsPopup = null;
	private TextView statsText = null;
	private Runnable pendingLoadingDialog = null;
	private TextView loadingText = null;
	private String loadingMessage = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		appContext = getApplicationContext();
		log(Log.INFO, TAG, "AliceActivity onCreate started. Android " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")");

		try {
			super.onCreate(savedInstanceState);
		} catch (Throwable t) {
			recordFatalCrash("NativeActivity super.onCreate() failed! ANativeActivity_onCreate symbol missing or native error.", t);
			showFatalCrashDialog(t);
			return;
		}

		hideSystemBars();
		checkForPreviousCrash();
	}

	private void showFatalCrashDialog(final Throwable t) {
		runOnUiThread(() -> {
			ScrollView scrollView = new ScrollView(this);
			int pad = (int) (16 * getResources().getDisplayMetrics().density);
			scrollView.setPadding(pad, pad, pad, pad);
			TextView tv = new TextView(this);
			tv.setTypeface(Typeface.MONOSPACE);
			tv.setTextSize(12);
			tv.setTextColor(0xFFFF5555);
			tv.setText("启动致命错误:\n" + Log.getStackTraceString(t));
			scrollView.addView(tv);

			new AlertDialog.Builder(this)
				.setTitle("⚠️ 维多利亚 2 启动致命错误")
				.setView(scrollView)
				.setCancelable(false)
				.setPositiveButton("📋 复制错误日志", (d, w) -> {
					ClipboardManager cm = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
					if (cm != null) {
						cm.setPrimaryClip(ClipData.newPlainText("vic2_fatal_error", Log.getStackTraceString(t)));
						Toast.makeText(this, "错误日志已复制到剪贴板！", Toast.LENGTH_LONG).show();
					}
					finish();
				})
				.setNegativeButton("退出", (d, w) -> finish())
				.show();
		});
	}

	private void checkForPreviousCrash() {
		File[] candidates = new File[] {
			new File(Environment.getExternalStorageDirectory(), "vic2_crash_log.txt"),
			getExternalFilesDir(null) != null ? new File(getExternalFilesDir(null), "vic2_crash_log.txt") : null,
			new File(getFilesDir(), "vic2_crash_log.txt")
		};

		File crashFile = null;
		String crashContent = "";
		for (File f : candidates) {
			if (f != null && f.exists() && f.length() > 0) {
				crashContent = readFileToString(f);
				if (!crashContent.trim().isEmpty()) {
					crashFile = f;
					break;
				}
			}
		}

		if (crashFile == null || crashContent.isEmpty()) {
			return;
		}

		final File targetCrashFile = crashFile;
		final String finalContent = crashContent;

		ScrollView scrollView = new ScrollView(this);
		int pad = (int) (16 * getResources().getDisplayMetrics().density);
		scrollView.setPadding(pad, pad, pad, pad);
		TextView tv = new TextView(this);
		tv.setTypeface(Typeface.MONOSPACE);
		tv.setTextSize(12);
		tv.setTextColor(0xFF333333);
		tv.setText(finalContent);
		scrollView.addView(tv);

		new AlertDialog.Builder(this)
			.setTitle("⚠️ 检测到游戏历史崩溃记录")
			.setView(scrollView)
			.setCancelable(false)
			.setPositiveButton("📋 复制日志", (d, w) -> {
				ClipboardManager cm = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
				if (cm != null) {
					cm.setPrimaryClip(ClipData.newPlainText("vic2_crash_log", finalContent));
					Toast.makeText(AliceActivity.this, "✅ 崩溃日志已复制到剪贴板！可以直接粘贴到聊天框发送。", Toast.LENGTH_LONG).show();
				}
			})
			.setNeutralButton("清空并启动", (d, w) -> {
				try { targetCrashFile.delete(); } catch (Throwable ignored) {}
				Toast.makeText(AliceActivity.this, "已清除崩溃日志", Toast.LENGTH_SHORT).show();
			})
			.setNegativeButton("继续启动", null)
			.show();
	}

	private void showLogViewerDialog() {
		File[] candidates = new File[] {
			new File(Environment.getExternalStorageDirectory(), "vic2_log.txt"),
			getExternalFilesDir(null) != null ? new File(getExternalFilesDir(null), "vic2_log.txt") : null,
			new File(getFilesDir(), "vic2_log.txt")
		};
		String logContent = "";
		for (File f : candidates) {
			if (f != null && f.exists() && f.length() > 0) {
				logContent = readFileToString(f);
				if (!logContent.isEmpty()) break;
			}
		}
		if (logContent.isEmpty()) {
			logContent = "(暂无运行日志记录)";
		}

		ScrollView scrollView = new ScrollView(this);
		int pad = (int) (16 * getResources().getDisplayMetrics().density);
		scrollView.setPadding(pad, pad, pad, pad);
		TextView tv = new TextView(this);
		tv.setTypeface(Typeface.MONOSPACE);
		tv.setTextSize(11);
		tv.setTextColor(0xFF333333);
		tv.setText(logContent);
		scrollView.addView(tv);

		final String copyText = logContent;
		new AlertDialog.Builder(this)
			.setTitle("📜 维多利亚 2 移动端运行日志")
			.setView(scrollView)
			.setPositiveButton("📋 复制全部日志", (d, w) -> {
				ClipboardManager cm = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
				if (cm != null) {
					cm.setPrimaryClip(ClipData.newPlainText("vic2_log", copyText));
					Toast.makeText(AliceActivity.this, "✅ 运行日志已复制到剪贴板！", Toast.LENGTH_LONG).show();
				}
			})
			.setNegativeButton("关闭", null)
			.show();
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus) {
			hideSystemBars();
		}
	}

	private void hideSystemBars() {
		Window window = getWindow();
		window.setDecorFitsSystemWindows(false);
		WindowInsetsController controller = window.getInsetsController();
		if (controller != null) {
			controller.hide(WindowInsets.Type.systemBars());
			controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
		}
	}

	@Override
	protected void onResume() {
		super.onResume();
		if (waitingForAllFilesAccess) {
			waitingForAllFilesAccess = false;
			if (Environment.isExternalStorageManager()) {
				launchFolderPicker();
			} else {
				showFolderDialog("未获得「所有文件访问权限」，维多利亚 2 移动端将无法读取端游游戏数据文件夹。");
			}
		}
	}

	public void requestGameFolder(final String reason) {
		runOnUiThread(() -> showFolderDialog(reason));
	}

	public void setLoading(final String message) {
		runOnUiThread(() -> {
			if (message == null || message.isEmpty()) {
				loadingMessage = null;
				if (pendingLoadingDialog != null) {
					uiHandler.removeCallbacks(pendingLoadingDialog);
					pendingLoadingDialog = null;
				}
				if (loadingDialog != null) {
					loadingDialog.dismiss();
					loadingDialog = null;
					loadingText = null;
				}
				return;
			}
			loadingMessage = message;
			if (loadingDialog != null) {
				loadingText.setText(message);
				return;
			}
			if (pendingLoadingDialog != null) {
				return;
			}
			pendingLoadingDialog = () -> {
				pendingLoadingDialog = null;
				showLoadingDialog(loadingMessage);
			};
			uiHandler.postDelayed(pendingLoadingDialog, 200);
		});
	}

	private void showLoadingDialog(String message) {
		if (isFinishing() || message == null) return;
		int padding = (int)(24 * getResources().getDisplayMetrics().density);
		LinearLayout layout = new LinearLayout(this);
		layout.setOrientation(LinearLayout.HORIZONTAL);
		layout.setGravity(Gravity.CENTER_VERTICAL);
		layout.setPadding(padding, padding, padding, padding);
		layout.setBackgroundColor(0xFF1E1812);

		ProgressBar spinner = new ProgressBar(this);
		spinner.setIndeterminate(true);
		layout.addView(spinner);

		loadingText = new TextView(this);
		loadingText.setText(message);
		loadingText.setTextSize(16);
		loadingText.setTextColor(0xFFE6D5B8);
		loadingText.setPadding(padding, 0, 0, 0);
		layout.addView(loadingText);

		loadingDialog = new AlertDialog.Builder(this)
			.setView(layout)
			.setCancelable(false)
			.create();
		loadingDialog.setCanceledOnTouchOutside(false);

		Window window = loadingDialog.getWindow();
		if (window != null) {
			window.setFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE, WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
		}
		loadingDialog.show();
		if (window != null) {
			WindowInsetsController controller = window.getInsetsController();
			if (controller != null) {
				controller.hide(WindowInsets.Type.systemBars());
				controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
			}
			window.clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
		}
	}

	public void setStats(final String text) {
		runOnUiThread(() -> {
			if (text == null || text.isEmpty()) {
				if (statsPopup != null) {
					statsPopup.dismiss();
					statsPopup = null;
					statsText = null;
				}
				return;
			}
			if (statsPopup == null) {
				if (isFinishing()) return;
				int padding = (int)(4 * getResources().getDisplayMetrics().density);
				statsText = new TextView(this);
				statsText.setTextColor(0xFFFFFFFF);
				statsText.setBackgroundColor(0xA0000000);
				statsText.setTypeface(Typeface.MONOSPACE);
				statsText.setTextSize(12);
				statsText.setPadding(padding, padding, padding, padding);
				statsPopup = new PopupWindow(statsText, ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT, false);
				statsPopup.setTouchable(false);
				statsText.setText(text);
				statsPopup.showAtLocation(getWindow().getDecorView(), Gravity.TOP | Gravity.END, padding, padding);
				return;
			}
			statsText.setText(text);
		});
	}

	public void showMessage(final String message) {
		runOnUiThread(() -> Toast.makeText(this, message, Toast.LENGTH_LONG).show());
	}

	private void showFolderDialog(String reason) {
		if (folderDialog != null && folderDialog.isShowing()) {
			folderDialog.dismiss();
		}
		String message = "维多利亚 2 移动端需要端游原版游戏数据（含扩展包）。\n\n"
			+ "请将维多利亚2游戏文件夹拷贝至手机存储中，并在接下来的系统选择器中选择该目录"
			+ "（目录内需包含 common、map 和 gfx 文件夹）。";
		if (reason != null && !reason.isEmpty()) {
			message = reason + "\n\n" + message;
		}
		folderDialog = new AlertDialog.Builder(this)
			.setTitle("选择维多利亚2游戏目录")
			.setMessage(message)
			.setCancelable(false)
			.setPositiveButton("📁 选择文件夹", (d, w) -> ensureAccessThenPick())
			.setNeutralButton("📜 查看运行日志", (d, w) -> showLogViewerDialog())
			.setNegativeButton("退出", (d, w) -> finish())
			.show();
	}

	private void ensureAccessThenPick() {
		if (Environment.isExternalStorageManager()) {
			launchFolderPicker();
			return;
		}
		waitingForAllFilesAccess = true;
		try {
			startActivity(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
				Uri.parse("package:" + getPackageName())));
		} catch (Exception e) {
			startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
		}
	}

	private void launchFolderPicker() {
		Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
		try {
			startActivityForResult(intent, REQUEST_PICK_FOLDER);
		} catch (Exception e) {
			showFolderDialog("无法打开当前设备的系统文件夹选择器。");
		}
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		if (requestCode != REQUEST_PICK_FOLDER) return;
		if (resultCode != RESULT_OK || data == null || data.getData() == null) {
			showFolderDialog("未选择任何文件夹。");
			return;
		}
		String path = treeUriToPath(data.getData());
		if (path == null) {
			showFolderDialog("请选择手机内部存储或物理SD卡上的本地文件夹（不支持云盘）。");
			return;
		}
		log(Log.INFO, TAG, "Game folder selected: " + path);
		nativeOnGameFolderPicked(path);
	}

	static String treeUriToPath(Uri uri) {
		if (!"com.android.externalstorage.documents".equals(uri.getAuthority())) {
			return null;
		}
		String docId;
		try {
			docId = DocumentsContract.getTreeDocumentId(uri);
		} catch (Exception e) {
			return null;
		}
		int colon = docId.indexOf(':');
		if (colon < 0) return null;
		String volume = docId.substring(0, colon);
		String relative = docId.substring(colon + 1);
		String base = "primary".equalsIgnoreCase(volume)
			? Environment.getExternalStorageDirectory().getAbsolutePath()
			: "/storage/" + volume;
		return relative.isEmpty() ? base : base + "/" + relative;
	}
}
