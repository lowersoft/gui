#include "ini.h"

#include <algorithm>
#include <cctype>

namespace gui::detail {

std::string Trim(const std::string& text) {
    const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    const auto first = std::find_if_not(text.begin(), text.end(), isSpace);
    const auto last  = std::find_if_not(text.rbegin(), text.rend(), isSpace).base();
    return first < last ? std::string(first, last) : std::string();
}

std::string ToLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

namespace {

// Removes a trailing "; comment". The ';' must start the value or follow whitespace.
std::string StripInlineComment(const std::string& value) {
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == ';' && (i == 0 || std::isspace(static_cast<unsigned char>(value[i - 1])))) {
            return Trim(value.substr(0, i));
        }
    }
    return value;
}

}  // namespace

IniDocument ParseIni(const std::string& text) {
    IniDocument doc;
    IniSection* current = nullptr;

    std::size_t position = 0;
    int lineNumber = 0;

    // Skip a UTF-8 byte order mark.
    if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) position = 3;

    while (position <= text.size()) {
        std::size_t end = text.find('\n', position);
        if (end == std::string::npos) end = text.size();
        const std::string line = Trim(text.substr(position, end - position));
        position = end + 1;
        ++lineNumber;

        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        if (line[0] == '[') {
            const std::size_t close = line.find(']');
            if (close == std::string::npos) {
                doc.errors.push_back({lineNumber, "unterminated section header"});
                continue;
            }
            doc.sections.push_back({Trim(line.substr(1, close - 1)), lineNumber, {}});
            current = &doc.sections.back();
            continue;
        }

        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) {
            doc.errors.push_back({lineNumber, "expected 'key = value'"});
            continue;
        }

        IniEntry entry;
        entry.key   = Trim(line.substr(0, equals));
        entry.value = StripInlineComment(Trim(line.substr(equals + 1)));
        entry.line  = lineNumber;
        if (entry.key.empty()) {
            doc.errors.push_back({lineNumber, "missing key before '='"});
            continue;
        }

        if (!current) {
            // Entries before the first header are kept in an unnamed section so the caller can report them.
            doc.sections.push_back({std::string(), lineNumber, {}});
            current = &doc.sections.back();
        }
        current->entries.push_back(std::move(entry));
    }
    return doc;
}

}  // namespace gui::detail
