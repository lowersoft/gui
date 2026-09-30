/// @file paths.h
/// @brief Locating the executable and resolving configuration files.
#pragma once

#include <filesystem>
#include <string>

namespace gui::detail {

/// Directory containing the running executable; empty if it cannot be determined.
std::filesystem::path ExecutableDirectory();

/// Resolves a relative configuration path such as "conf/theme.ini".
/// Absolute paths are returned unchanged. Otherwise the first existing candidate wins:
///   1. the source tree (only in builds that define GUI_DEV_ROOT, i.e. Debug)
///   2. the executable's directory
///   3. the current working directory
/// When nothing exists, the executable-relative path is returned so error messages are meaningful.
std::string ResolveConfigPath(const std::string& path);

/// Last write time of a file as an opaque, comparable value; 0 if the file is missing.
long long FileTimestamp(const std::string& path);

}  // namespace gui::detail
