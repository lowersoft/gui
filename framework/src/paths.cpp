#include "paths.h"

#include <cstdint>
#include <system_error>
#include <vector>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#endif

namespace fs = std::filesystem;

namespace gui::detail {

fs::path ExecutableDirectory() {
#if defined(_WIN32)
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) return {};
        if (length < buffer.size()) return fs::path(std::wstring(buffer.data(), length)).parent_path();
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) return {};
    std::error_code ec;
    const fs::path canonical = fs::canonical(buffer.data(), ec);
    return ec ? fs::path(buffer.data()).parent_path() : canonical.parent_path();
#else
    std::error_code ec;
    const fs::path exe = fs::read_symlink("/proc/self/exe", ec);
    return ec ? fs::path() : exe.parent_path();
#endif
}

std::string ResolveConfigPath(const std::string& path) {
    const fs::path requested(path);
    if (requested.is_absolute()) return path;

    std::vector<fs::path> candidates;
#if defined(GUI_DEV_ROOT)
    candidates.push_back(fs::path(GUI_DEV_ROOT) / requested);
#endif
    const fs::path exeDir = ExecutableDirectory();
    if (!exeDir.empty()) candidates.push_back(exeDir / requested);
    candidates.push_back(fs::current_path() / requested);

    for (const fs::path& candidate : candidates) {
        std::error_code ec;
        if (fs::exists(candidate, ec)) return candidate.string();
    }
    return (exeDir.empty() ? requested : exeDir / requested).string();
}

long long FileTimestamp(const std::string& path) {
    std::error_code ec;
    const auto time = fs::last_write_time(path, ec);
    return ec ? 0 : static_cast<long long>(time.time_since_epoch().count());
}

}  // namespace gui::detail
