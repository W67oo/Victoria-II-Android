#pragma once

#include <android/log.h>
#include <csignal>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mutex>
#include <string>

namespace vic2_log {

inline std::string g_primary_log_path = "/storage/emulated/0/vic2_log.txt";
inline std::string g_fallback_log_path = "";
inline std::string g_primary_crash_path = "/storage/emulated/0/vic2_crash_log.txt";
inline std::string g_fallback_crash_path = "";
inline std::mutex g_log_mutex;

// Raw async-signal-safe string writer
inline void safe_write_str(int fd, const char* s) {
    if (fd >= 0 && s) {
        size_t len = 0;
        while (s[len]) ++len;
        (void)write(fd, s, len);
    }
}

// Raw async-signal-safe hex integer writer
inline void safe_write_hex(int fd, uintptr_t val) {
    if (fd < 0) return;
    char buf[2 + sizeof(val) * 2 + 1];
    buf[0] = '0';
    buf[1] = 'x';
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = (int)(sizeof(val) * 2) - 1; i >= 0; --i) {
        buf[2 + i] = hex_chars[val & 0xF];
        val >>= 4;
    }
    buf[2 + sizeof(val) * 2] = '\0';
    safe_write_str(fd, buf);
}

// Raw async-signal-safe integer writer
inline void safe_write_int(int fd, int val) {
    if (fd < 0) return;
    if (val < 0) {
        safe_write_str(fd, "-");
        val = -val;
    }
    char buf[16];
    int idx = 14;
    buf[15] = '\0';
    if (val == 0) {
        buf[idx--] = '0';
    } else {
        while (val > 0 && idx >= 0) {
            buf[idx--] = '0' + (val % 10);
            val /= 10;
        }
    }
    safe_write_str(fd, &buf[idx + 1]);
}

// POSIX Crash Signal Handler for SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL
inline void native_crash_signal_handler(int sig, siginfo_t* info, void* ucontext) {
    const char* sig_name = "UNKNOWN_SIGNAL";
    switch (sig) {
        case SIGSEGV: sig_name = "SIGSEGV (Segmentation Fault - 非法内存访问)"; break;
        case SIGBUS:  sig_name = "SIGBUS (Bus Error - 总线错误/未对齐访问)"; break;
        case SIGABRT: sig_name = "SIGABRT (Abort - 异常终止/断言失败)"; break;
        case SIGFPE:  sig_name = "SIGFPE (Floating Point Exception - 浮点算术异常)"; break;
        case SIGILL:  sig_name = "SIGILL (Illegal Instruction - 非法指令)"; break;
    }

    int fds[2];
    int num_fds = 0;
    int fd1 = open(g_primary_crash_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd1 >= 0) fds[num_fds++] = fd1;
    if (!g_fallback_crash_path.empty()) {
        int fd2 = open(g_fallback_crash_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (fd2 >= 0) fds[num_fds++] = fd2;
    }

    for (int i = 0; i < num_fds; ++i) {
        int fd = fds[i];
        safe_write_str(fd, "\n=======================================================\n");
        safe_write_str(fd, "[FATAL NATIVE CRASH DETECTED]\n");
        safe_write_str(fd, "Signal: ");
        safe_write_str(fd, sig_name);
        safe_write_str(fd, " (signal=");
        safe_write_int(fd, sig);
        safe_write_str(fd, ")\n");
        if (info) {
            safe_write_str(fd, "Fault address: ");
            safe_write_hex(fd, reinterpret_cast<uintptr_t>(info->si_addr));
            safe_write_str(fd, "\nSignal code: ");
            safe_write_int(fd, info->si_code);
            safe_write_str(fd, "\n");
        }
        safe_write_str(fd, "Thread ID: ");
        safe_write_int(fd, (int)gettid());
        safe_write_str(fd, "\n=======================================================\n");
        fsync(fd);
        close(fd);
    }

    __android_log_print(ANDROID_LOG_FATAL, "Vic2Crash", "Fatal signal %d (%s) at address %p (tid=%d)",
                        sig, sig_name, info ? info->si_addr : nullptr, (int)gettid());

    signal(sig, SIG_DFL);
    raise(sig);
}

inline void init_crash_handler(const char* base_dir) {
    if (base_dir && base_dir[0]) {
        g_fallback_log_path = std::string(base_dir) + "/vic2_log.txt";
        g_fallback_crash_path = std::string(base_dir) + "/vic2_crash_log.txt";
    }

    // Set up alternate signal stack so stack exhaustion in C++ doesn't kill the handler
    stack_t ss;
    static char alt_stack[SIGSTKSZ * 4];
    ss.ss_sp = alt_stack;
    ss.ss_size = sizeof(alt_stack);
    ss.ss_flags = 0;
    sigaltstack(&ss, nullptr);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = native_crash_signal_handler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
}

inline void write_log(int priority, const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    __android_log_vprint(priority, tag, fmt, args);
    va_end(args);

    char msg_buf[2048];
    va_start(args, fmt);
    vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    va_end(args);

    time_t raw_time = time(nullptr);
    struct tm time_info;
    localtime_r(&raw_time, &time_info);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &time_info);

    const char* prio_str = "INFO";
    if (priority == ANDROID_LOG_ERROR) prio_str = "ERROR";
    else if (priority == ANDROID_LOG_WARN) prio_str = "WARN";
    else if (priority == ANDROID_LOG_DEBUG) prio_str = "DEBUG";
    else if (priority == ANDROID_LOG_FATAL) prio_str = "FATAL";

    char line_buf[2304];
    int line_len = snprintf(line_buf, sizeof(line_buf), "[%s] [%s/%s] %s\n",
                            time_buf, tag ? tag : "Vic2", prio_str, msg_buf);

    std::lock_guard<std::mutex> lock(g_log_mutex);
    // 1. Write to app-internal fallback log (guaranteed writable without permissions)
    if (!g_fallback_log_path.empty()) {
        FILE* f = fopen(g_fallback_log_path.c_str(), "a");
        if (f) {
            fwrite(line_buf, 1, line_len, f);
            fflush(f);
            fclose(f);
        }
    }
    // 2. Write to primary public log (/storage/emulated/0/vic2_log.txt)
    FILE* f = fopen(g_primary_log_path.c_str(), "a");
    if (f) {
        fwrite(line_buf, 1, line_len, f);
        fflush(f);
        fclose(f);
    }
}

} // namespace vic2_log

#define LAUNCHER_LOGI(...) vic2_log::write_log(ANDROID_LOG_INFO, "Vic2Launcher", __VA_ARGS__)
#define LAUNCHER_LOGE(...) vic2_log::write_log(ANDROID_LOG_ERROR, "Vic2Launcher", __VA_ARGS__)
#define ALICE_LOGI(...)    vic2_log::write_log(ANDROID_LOG_INFO, "Vic2Window", __VA_ARGS__)
#define ALICE_LOGE(...)    vic2_log::write_log(ANDROID_LOG_ERROR, "Vic2Window", __VA_ARGS__)
