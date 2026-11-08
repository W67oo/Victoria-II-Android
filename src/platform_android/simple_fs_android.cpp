// =============================================================================
// simple_fs_android.cpp
// POSIX mmap + Caseless File Lookup + App-Private Sandbox Directory Routing
// for Android arm64-v8a (ext4 / f2fs / fuse safe, zero getenv("HOME") crash).
// =============================================================================

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstring>

#include "runtime_config_android.hpp"
#include "simple_fs.hpp"
#include "text.hpp"

namespace simple_fs {

namespace {

inline char ascii_tolower(char c) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

inline native_string normalize_rel_slashes(native_string_view sv) {
    native_string out;
    out.reserve(sv.size());
    for (char c : sv) {
        char nc = (c == '\\') ? '/' : c;
        if (nc == '/' && !out.empty() && out.back() == '/') continue;
        out.push_back(nc);
    }
    while (!out.empty() && out.front() == '/') out.erase(out.begin());
    return out;
}

inline native_string join_path(native_string_view a, native_string_view b) {
    native_string rel = normalize_rel_slashes(b);
    if (a.empty()) return rel;
    native_string res(a);
    for (char& c : res) {
        if (c == '\\') c = '/';
    }
    if (rel.empty()) return res;
    if (res.back() != '/') res.push_back('/');
    res += rel;
    return res;
}

inline bool is_regular_entry(const std::string& parent_dir, const struct dirent* ent) noexcept {
    if (ent->d_type == DT_REG) return true;
    if (ent->d_type != DT_UNKNOWN && ent->d_type != DT_LNK) return false;
    struct stat sb {};
    std::string full = join_path(parent_dir, ent->d_name);
    return stat(full.c_str(), &sb) == 0 && S_ISREG(sb.st_mode);
}

inline bool is_dir_entry(const std::string& parent_dir, const struct dirent* ent) noexcept {
    if (ent->d_type == DT_DIR) return true;
    if (ent->d_type != DT_UNKNOWN && ent->d_type != DT_LNK) return false;
    struct stat sb {};
    std::string full = join_path(parent_dir, ent->d_name);
    return stat(full.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
}

inline bool contains_non_ascii(const char* str) noexcept {
    for (const auto* c = reinterpret_cast<const unsigned char*>(str); *c != 0; ++c) {
        if (*c > 127) return true;
    }
    return false;
}

void ensure_directories_recursive(const native_string& dir_path) {
    if (dir_path.empty()) return;
    native_string tmp = dir_path;
    for (char& c : tmp) {
        if (c == '\\') c = '/';
    }
    mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP;
    for (size_t i = 1; i < tmp.size(); ++i) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';
            mkdir(tmp.c_str(), mode);
            tmp[i] = '/';
        }
    }
    mkdir(tmp.c_str(), mode);
}

std::optional<unopened_file> find_file_caseless(const directory& dir, native_string_view file_name) {
    native_string target = normalize_rel_slashes(file_name);
    // Support nested relative path such as "interface/icon_money_big.dds"
    size_t slash_pos = target.find_last_of('/');
    if (slash_pos != native_string::npos) {
        directory sub = open_directory(dir, std::string_view(target).substr(0, slash_pos));
        return find_file_caseless(sub, std::string_view(target).substr(slash_pos + 1));
    }

    std::transform(target.begin(), target.end(), target.begin(), ascii_tolower);
    for (const auto& candidate : list_files(dir, NATIVE(""))) {
        native_string cand_name = get_file_name(candidate);
        std::transform(cand_name.begin(), cand_name.end(), cand_name.begin(), ascii_tolower);
        if (cand_name == target) {
            return candidate;
        }
    }
    return std::nullopt;
}

} // namespace

file::~file() {
#if defined(_GNU_SOURCE) || defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
    if (mapping_handle && mapping_handle != MAP_FAILED && content.file_size > 0) {
        munmap(mapping_handle, content.file_size);
    }
#else
    if (file_buffer) {
        free(file_buffer);
    }
#endif
    if (file_descriptor != -1) {
        close(file_descriptor);
    }
}

file::file(file&& other) noexcept {
    file_descriptor = other.file_descriptor;
#if defined(_GNU_SOURCE) || defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
    mapping_handle = other.mapping_handle;
    other.mapping_handle = nullptr;
#else
    file_buffer = other.file_buffer;
    other.file_buffer = nullptr;
#endif
    content = other.content;
    absolute_path = std::move(other.absolute_path);
    other.file_descriptor = -1;
    other.content = {};
}

void file::operator=(file&& other) noexcept {
    if (this == &other) return;
#if defined(_GNU_SOURCE) || defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
    if (mapping_handle && mapping_handle != MAP_FAILED && content.file_size > 0) {
        munmap(mapping_handle, content.file_size);
    }
    mapping_handle = other.mapping_handle;
    other.mapping_handle = nullptr;
#else
    if (file_buffer) free(file_buffer);
    file_buffer = other.file_buffer;
    other.file_buffer = nullptr;
#endif
    if (file_descriptor != -1) close(file_descriptor);
    file_descriptor = other.file_descriptor;
    content = other.content;
    absolute_path = std::move(other.absolute_path);
    other.file_descriptor = -1;
    other.content = {};
}

file::file(native_string const& full_path) {
    static constexpr char kEmptyFileByte = '\0';
    file_descriptor = open(full_path.c_str(), O_RDONLY);
    if (file_descriptor != -1) {
        absolute_path = full_path;
        struct stat sb {};
        if (fstat(file_descriptor, &sb) != -1) {
            content.file_size = static_cast<uint32_t>(sb.st_size);
            if (content.file_size == 0) {
                content.data = &kEmptyFileByte;
                return;
            }
#if defined(_GNU_SOURCE) || defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
            mapping_handle = mmap(nullptr, content.file_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
            if (mapping_handle != MAP_FAILED) {
                content.data = static_cast<const char*>(mapping_handle);
            }
#else
            file_buffer = malloc(content.file_size);
            if (file_buffer && read(file_descriptor, file_buffer, content.file_size) >= 0) {
                content.data = static_cast<const char*>(file_buffer);
            }
#endif
        }
    }
}

file::file(int fd, native_string const& full_path) : file_descriptor(fd) {
    static constexpr char kEmptyFileByte = '\0';
    absolute_path = full_path;
    struct stat sb {};
    if (fstat(file_descriptor, &sb) != -1) {
        content.file_size = static_cast<uint32_t>(sb.st_size);
        if (content.file_size == 0) {
            content.data = &kEmptyFileByte;
            return;
        }
#if defined(_GNU_SOURCE) || defined(_DEFAULT_SOURCE) || defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
        mapping_handle = mmap(nullptr, content.file_size, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
        if (mapping_handle != MAP_FAILED) {
            content.data = static_cast<const char*>(mapping_handle);
        }
#else
        file_buffer = malloc(content.file_size);
        if (file_buffer && read(file_descriptor, file_buffer, content.file_size) >= 0) {
            content.data = static_cast<const char*>(file_buffer);
        }
#endif
    }
}

std::optional<file> open_file(unopened_file const& f) {
    std::optional<file> result(file{f.absolute_path});
    if (!result->content.data) {
        result.reset();
    }
    return result;
}

void reset(file_system& fs) {
    fs.ordered_roots.clear();
    fs.ignored_paths.clear();
}

void add_root(file_system& fs, native_string_view root_path) {
    native_string clean(root_path);
    for (char& c : clean) {
        if (c == '\\') c = '/';
    }
    while (clean.size() > 1 && clean.back() == '/') clean.pop_back();
    fs.ordered_roots.emplace_back(std::move(clean));
}

void add_relative_root(file_system& fs, native_string_view root_path) {
    std::string base;
    {
        std::lock_guard<std::mutex> lk(vic2_mobile::runtime::config().mtx);
        base = vic2_mobile::runtime::config().vic2_assets_root;
    }
    if (base.empty()) base = ".";
    add_root(fs, join_path(base, root_path));
}

directory get_root(file_system const& fs) {
    return directory(&fs, NATIVE(""));
}

native_string extract_state(file_system const& fs) {
    native_string result;
    for (auto const& str : fs.ordered_roots) {
        result += NATIVE(";") + str;
    }
    result += NATIVE("?");
    for (auto const& replace_path : fs.ignored_paths) {
        result += replace_path + NATIVE(";");
    }
    return result;
}

void restore_state(file_system& fs, native_string_view data) {
    simple_fs::reset(fs);
    if (data.empty()) return;
    auto break_position = std::find(data.data(), data.data() + data.length(), NATIVE('?'));
    {
        auto position = data.data() + 1;
        auto end = break_position;
        while (position < end) {
            auto next_semicolon = std::find(position, end, NATIVE(';'));
            fs.ordered_roots.emplace_back(position, next_semicolon);
            position = next_semicolon + 1;
        }
    }
    {
        auto position = break_position + 1;
        auto end = data.data() + data.length();
        while (position < end) {
            auto next_semicolon = std::find(position, end, NATIVE(';'));
            fs.ignored_paths.emplace_back(position, next_semicolon);
            position = next_semicolon + 1;
        }
    }
}

std::vector<unopened_file> list_files(directory const& dir, native_char const* extension) {
    std::vector<unopened_file> accumulated_results;
    auto match_ext = [&](const char* name) -> bool {
        if (!extension || extension[0] == 0) return true;
        const char* dot = std::strrchr(name, '.');
        if (!dot || dot == name) return false;
        size_t len_a = std::strlen(dot);
        size_t len_b = std::strlen(extension);
        if (len_a != len_b) return false;
        for (size_t k = 0; k < len_a; ++k) {
            if (ascii_tolower(dot[k]) != ascii_tolower(extension[k])) return false;
        }
        return true;
    };

    if (dir.parent_system) {
        for (size_t i = dir.parent_system->ordered_roots.size(); i-- > 0;) {
            auto const appended_path = join_path(dir.parent_system->ordered_roots[i], dir.relative_path);
            if (simple_fs::is_ignored_path(*dir.parent_system, appended_path + NATIVE("/"))) {
                continue;
            }
            DIR* d = opendir(appended_path.c_str());
            if (d) {
                struct dirent* dir_ent = nullptr;
                while ((dir_ent = readdir(d)) != nullptr) {
                    if (!is_regular_entry(appended_path, dir_ent)) continue;
                    if (!match_ext(dir_ent->d_name)) continue;
                    if (contains_non_ascii(dir_ent->d_name)) continue;

                    auto existing = std::find_if(
                        accumulated_results.begin(), accumulated_results.end(),
                        [n = dir_ent->d_name](auto const& f) { return f.file_name == n; });
                    if (existing == accumulated_results.end()) {
                        accumulated_results.emplace_back(join_path(appended_path, dir_ent->d_name), dir_ent->d_name);
                    }
                }
                closedir(d);
            }
        }
    } else {
        DIR* d = opendir(dir.relative_path.c_str());
        if (d) {
            struct dirent* dir_ent = nullptr;
            while ((dir_ent = readdir(d)) != nullptr) {
                if (!is_regular_entry(dir.relative_path, dir_ent)) continue;
                if (!match_ext(dir_ent->d_name)) continue;
                if (contains_non_ascii(dir_ent->d_name)) continue;
                accumulated_results.emplace_back(join_path(dir.relative_path, dir_ent->d_name), dir_ent->d_name);
            }
            closedir(d);
        }
    }

    std::sort(accumulated_results.begin(), accumulated_results.end(), [](unopened_file const& a, unopened_file const& b) {
        return std::lexicographical_compare(
            a.file_name.begin(), a.file_name.end(),
            b.file_name.begin(), b.file_name.end(),
            [](char c1, char c2) { return ascii_tolower(c1) < ascii_tolower(c2); });
    });
    return accumulated_results;
}

std::vector<directory> list_subdirectories(directory const& dir) {
    std::vector<directory> accumulated_results;
    if (dir.parent_system) {
        for (size_t i = dir.parent_system->ordered_roots.size(); i-- > 0;) {
            auto const appended_path = join_path(dir.parent_system->ordered_roots[i], dir.relative_path);
            if (simple_fs::is_ignored_path(*dir.parent_system, appended_path + NATIVE("/"))) {
                continue;
            }
            DIR* d = opendir(appended_path.c_str());
            if (d) {
                struct dirent* dir_ent = nullptr;
                while ((dir_ent = readdir(d)) != nullptr) {
                    if (dir_ent->d_name[0] == '.') continue;
                    if (!is_dir_entry(appended_path, dir_ent)) continue;
                    if (contains_non_ascii(dir_ent->d_name)) continue;

                    native_string const rel_name = dir.relative_path + NATIVE("/") + dir_ent->d_name;
                    auto existing = std::find_if(
                        accumulated_results.begin(), accumulated_results.end(),
                        [&rel_name](auto const& s) { return s.relative_path == rel_name; });
                    if (existing == accumulated_results.end()) {
                        accumulated_results.emplace_back(dir.parent_system, rel_name);
                    }
                }
                closedir(d);
            }
        }
    } else {
        DIR* d = opendir(dir.relative_path.c_str());
        if (d) {
            struct dirent* dir_ent = nullptr;
            while ((dir_ent = readdir(d)) != nullptr) {
                if (dir_ent->d_name[0] == '.') continue;
                if (!is_dir_entry(dir.relative_path, dir_ent)) continue;
                if (contains_non_ascii(dir_ent->d_name)) continue;
                accumulated_results.emplace_back(nullptr, join_path(dir.relative_path, dir_ent->d_name));
            }
            closedir(d);
        }
    }

    std::sort(accumulated_results.begin(), accumulated_results.end(), [](directory const& a, directory const& b) {
        return std::lexicographical_compare(
            a.relative_path.begin(), a.relative_path.end(),
            b.relative_path.begin(), b.relative_path.end(),
            [](char c1, char c2) { return ascii_tolower(c1) < ascii_tolower(c2); });
    });
    return accumulated_results;
}

directory open_directory(directory const& dir, native_string_view directory_name) {
    native_string clean_sub = normalize_rel_slashes(directory_name);
    if (dir.parent_system) {
        if (clean_sub.empty()) return dir;
        return directory(dir.parent_system, dir.relative_path + NATIVE('/') + clean_sub);
    }
    return directory(nullptr, join_path(dir.relative_path, clean_sub));
}

native_string get_mod_save_dir_name(const simple_fs::file_system& fs) {
    native_string save_dir;
    for (const auto& root : simple_fs::list_roots(fs)) {
        size_t found = root.find_last_of('/');
        if (found != native_string::npos) {
            save_dir += root.substr(found + 1);
        }
    }
    return save_dir;
}

std::optional<file> open_file(directory const& dir, native_string_view file_name) {
    native_string clean_name = normalize_rel_slashes(file_name);
    if (dir.parent_system) {
        for (size_t i = dir.parent_system->ordered_roots.size(); i-- > 0;) {
            native_string dir_path = join_path(dir.parent_system->ordered_roots[i], dir.relative_path);
            native_string full_path = join_path(dir_path, clean_name);
            if (simple_fs::is_ignored_path(*dir.parent_system, full_path)) continue;
            int fd = open(full_path.c_str(), O_RDONLY);
            if (fd != -1) {
                std::optional<file> res(file(fd, full_path));
                if (res->content.data) return res;
            }
        }
    } else {
        native_string full_path = join_path(dir.relative_path, clean_name);
        int fd = open(full_path.c_str(), O_RDONLY);
        if (fd != -1) {
            std::optional<file> res(file(fd, full_path));
            if (res->content.data) return res;
        }
    }

    if (auto caseless = find_file_caseless(dir, clean_name); caseless) {
        return open_file(*caseless);
    }
    return std::nullopt;
}

std::optional<file> open_file(directory const& dir, std::vector<native_string_view> file_names) {
    for (auto fn : file_names) {
        if (auto f = open_file(dir, fn); f) return f;
    }
    return std::nullopt;
}

std::optional<unopened_file> peek_file(directory const& dir, native_string_view file_name) {
    native_string clean_name = normalize_rel_slashes(file_name);
    if (dir.parent_system) {
        for (size_t i = dir.parent_system->ordered_roots.size(); i-- > 0;) {
            native_string dir_path = join_path(dir.parent_system->ordered_roots[i], dir.relative_path);
            native_string full_path = join_path(dir_path, clean_name);
            if (simple_fs::is_ignored_path(*dir.parent_system, full_path)) continue;
            struct stat sb {};
            if (stat(full_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
                return unopened_file(full_path, clean_name);
            }
        }
    } else {
        native_string full_path = join_path(dir.relative_path, clean_name);
        struct stat sb {};
        if (stat(full_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
            return unopened_file(full_path, clean_name);
        }
    }
    return find_file_caseless(dir, clean_name);
}

void add_ignore_path(file_system& fs, native_string_view replaced_path) {
    fs.ignored_paths.emplace_back(correct_slashes(replaced_path));
}

std::vector<native_string> list_roots(file_system const& fs) {
    return fs.ordered_roots;
}

bool is_ignored_path(file_system const& fs, native_string_view path) {
    for (auto const& replace_path : fs.ignored_paths) {
        if (!replace_path.empty() && path.starts_with(replace_path)) return true;
    }
    return false;
}

native_string get_full_name(unopened_file const& f) { return f.absolute_path; }
native_string get_file_name(unopened_file const& f) { return f.file_name; }
native_string get_full_name(file const& f)          { return f.absolute_path; }
native_string get_full_name(directory const& dir)   { return dir.relative_path; }

native_string get_dir_name(directory const& dir) {
    size_t found = dir.relative_path.find_last_of('/');
    return (found == native_string::npos) ? dir.relative_path : dir.relative_path.substr(found + 1);
}

void write_file(directory const& dir, native_string_view file_name, char const* file_data, uint32_t file_size) {
    if (dir.parent_system) std::abort();
    ensure_directories_recursive(dir.relative_path);
    native_string full_path = join_path(dir.relative_path, file_name);
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP;
    int fd = open(full_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, mode);
    if (fd != -1) {
        int64_t rem = file_size;
        const char* p = file_data;
        while (rem > 0) {
            ssize_t w = write(fd, p, static_cast<size_t>(rem));
            if (w <= 0) break;
            p += w;
            rem -= w;
        }
        close(fd);
    }
}

void append_file(directory const& dir, native_string_view file_name, char const* file_data, uint32_t file_size) {
    if (dir.parent_system) std::abort();
    ensure_directories_recursive(dir.relative_path);
    native_string full_path = join_path(dir.relative_path, file_name);
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP;
    int fd = open(full_path.c_str(), O_RDWR | O_CREAT | O_APPEND, mode);
    if (fd != -1) {
        int64_t rem = file_size;
        const char* p = file_data;
        while (rem > 0) {
            ssize_t w = write(fd, p, static_cast<size_t>(rem));
            if (w <= 0) break;
            p += w;
            rem -= w;
        }
        close(fd);
    }
}

file_contents view_contents(file const& f) {
    return f.content;
}

// App-private sandbox directories on Android (replaces Linux ~/.local/share/Alice/)
directory get_or_create_root_documents() {
    native_string path = vic2_mobile::runtime::config().get_storage_root();
    ensure_directories_recursive(path);
    return directory(nullptr, path);
}

directory get_or_create_settings_directory() {
    native_string path = vic2_mobile::runtime::config().get_storage_root() + "settings/";
    ensure_directories_recursive(path);
    return directory(nullptr, path);
}

directory get_or_create_save_game_directory(native_string mod_dir) {
    native_string path = vic2_mobile::runtime::config().get_storage_root() + "saves/";
    if (!mod_dir.empty()) {
        path += normalize_rel_slashes(mod_dir) + "/";
    }
    ensure_directories_recursive(path);
    return directory(nullptr, path);
}

directory get_or_create_templates_directory() {
    native_string path = vic2_mobile::runtime::config().get_storage_root() + "templates/";
    ensure_directories_recursive(path);
    return directory(nullptr, path);
}

directory get_or_create_gamerules_directory() {
    native_string path = vic2_mobile::runtime::config().get_storage_root() + "gamerules/";
    ensure_directories_recursive(path);
    return directory(nullptr, path);
}

directory get_or_create_oos_directory() {
    native_string path = vic2_mobile::runtime::config().get_storage_root() + "oos/";
    ensure_directories_recursive(path);
    return directory(nullptr, path);
}

directory get_or_create_data_dumps_directory() {
    native_string path = vic2_mobile::runtime::config().get_storage_root() + "data_dumps/";
    ensure_directories_recursive(path);
    return directory(nullptr, path);
}

directory get_or_create_scenario_directory() {
    native_string path = vic2_mobile::runtime::config().get_storage_root() + "scenarios/";
    ensure_directories_recursive(path);
    return directory(nullptr, path);
}

// Encoding & path helpers (zero deprecated std::wstring_convert)
std::string utf16_to_utf8(std::u16string_view str) {
    std::string result;
    result.reserve(str.size() * 2);
    size_t pos = 0;
    while (pos < str.length()) {
        uint16_t u0 = static_cast<uint16_t>(str[pos]);
        if (u0 >= 0xDC00 && u0 <= 0xDFFF) {
            ++pos;
            continue;
        }
        uint32_t cp = u0;
        if (u0 >= 0xD800 && u0 <= 0xDBFF && pos + 1 < str.length()) {
            uint16_t u1 = static_cast<uint16_t>(str[pos + 1]);
            if (u1 >= 0xDC00 && u1 <= 0xDFFF) {
                cp = 0x10000u + ((static_cast<uint32_t>(u0 - 0xD800) << 10) | static_cast<uint32_t>(u1 - 0xDC00));
                ++pos;
            }
        }
        if (cp <= 0x7Fu) {
            result.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FFu) {
            result.push_back(static_cast<char>(0xC0u | ((cp >> 6) & 0x1Fu)));
            result.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        } else if (cp <= 0xFFFFu) {
            result.push_back(static_cast<char>(0xE0u | ((cp >> 12) & 0x0Fu)));
            result.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
            result.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        } else {
            result.push_back(static_cast<char>(0xF0u | ((cp >> 18) & 0x07u)));
            result.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
            result.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
            result.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        }
        ++pos;
    }
    return result;
}

native_string win1250_to_native(std::string_view data_in) {
    std::string result;
    result.reserve(data_in.size());
    for (char ch : data_in) {
        if (static_cast<signed char>(ch) >= 0) {
            result.push_back(ch);
        } else {
            char16_t u16 = text::win1250toUTF16(ch);
            result += utf16_to_utf8(std::u16string_view(&u16, 1));
        }
    }
    return result;
}

native_string utf8_to_native(std::string_view str)     { return std::string(str); }
std::string   native_to_utf8(native_string_view str)   { return std::string(str); }
native_string utf16_to_native(std::u16string_view str) { return utf16_to_utf8(str); }

std::u16string utf8_to_utf16(std::string_view str) {
    std::u16string result;
    result.reserve(str.size());
    const auto* p = reinterpret_cast<const uint8_t*>(str.data());
    const auto* end = p + str.size();
    while (p < end) {
        uint32_t cp = 0;
        if ((*p & 0x80u) == 0) {
            cp = *p++;
        } else if ((*p & 0xE0u) == 0xC0u && p + 1 < end) {
            cp = (static_cast<uint32_t>(p[0] & 0x1Fu) << 6) | static_cast<uint32_t>(p[1] & 0x3Fu);
            p += 2;
        } else if ((*p & 0xF0u) == 0xE0u && p + 2 < end) {
            cp = (static_cast<uint32_t>(p[0] & 0x0Fu) << 12) |
                 (static_cast<uint32_t>(p[1] & 0x3Fu) << 6) |
                 static_cast<uint32_t>(p[2] & 0x3Fu);
            p += 3;
        } else if ((*p & 0xF8u) == 0xF0u && p + 3 < end) {
            cp = (static_cast<uint32_t>(p[0] & 0x07u) << 18) |
                 (static_cast<uint32_t>(p[1] & 0x3Fu) << 12) |
                 (static_cast<uint32_t>(p[2] & 0x3Fu) << 6) |
                 static_cast<uint32_t>(p[3] & 0x3Fu);
            p += 4;
        } else {
            ++p;
            continue;
        }
        if (cp >= 0x10000u) {
            uint32_t v = cp - 0x10000u;
            result.push_back(static_cast<char16_t>(0xD800u | ((v >> 10) & 0x3FFu)));
            result.push_back(static_cast<char16_t>(0xDC00u | (v & 0x3FFu)));
        } else {
            result.push_back(static_cast<char16_t>(cp));
        }
    }
    return result;
}

std::string remove_double_backslashes(std::string_view data_in) {
    std::string res;
    res.reserve(data_in.size());
    for (size_t i = 0; i < data_in.size(); ++i) {
        if (data_in[i] == '\\') {
            res.push_back('/');
            if (i + 1 < data_in.size() && data_in[i + 1] == '\\') ++i;
        } else {
            res.push_back(data_in[i]);
        }
    }
    return res;
}

native_string remove_file_extension(const native_string& str) {
    size_t found = str.find_last_of('.');
    return (found != native_string::npos) ? str.substr(0, found) : str;
}

native_string correct_slashes(native_string_view path) {
    std::string res(path);
    for (char& c : res) {
        if (c == '\\') c = '/';
    }
    return res;
}

void fileseperators_from_native_to_standard(std::vector<char>& input) {
    for (char& c : input) {
        if (c == '/') c = '\\';
    }
}

void fileseperators_from_standard_to_native(std::vector<char>& input) {
    for (char& c : input) {
        if (c == '\\') c = '/';
    }
}

std::vector<char> fileseperators_from_native_to_standard_copy(const std::vector<char>& input) {
    std::vector<char> result(input);
    fileseperators_from_native_to_standard(result);
    return result;
}

std::vector<char> fileseperators_from_standard_to_native_copy(const std::vector<char>& input) {
    std::vector<char> result(input);
    fileseperators_from_standard_to_native(result);
    return result;
}

void standardize_newlines(std::string& input) {
    input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
}

} // namespace simple_fs
