/// @file ini.h
/// @brief Minimal INI reader used by the theme loader.
///
/// Supported syntax:
///   - `[section]` headers and `key = value` pairs
///   - full-line comments starting with ';' or '#'
///   - inline comments introduced by ';' preceded by whitespace
/// Values are returned verbatim (trimmed); interpretation is left to the caller.
#pragma once

#include <string>
#include <vector>

namespace gui::detail {

struct IniEntry {
    std::string key;
    std::string value;
    int         line = 0;
};

struct IniSection {
    std::string name;
    int         line = 0;
    std::vector<IniEntry> entries;
};

struct IniError {
    int         line = 0;
    std::string message;
};

struct IniDocument {
    std::vector<IniSection> sections;   ///< In file order; repeated headers produce separate sections.
    std::vector<IniError>   errors;     ///< Syntax problems; parsing continues past them.
};

IniDocument ParseIni(const std::string& text);

std::string Trim(const std::string& text);
std::string ToLower(std::string text);

}  // namespace gui::detail
