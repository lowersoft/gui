#include "gui/theme.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "ini.h"

namespace gui {
namespace {

using detail::ToLower;
using detail::Trim;

struct FloatField {
    const char*          name;
    float ImGuiStyle::*  member;
};

struct Vec2Field {
    const char*          name;
    ImVec2 ImGuiStyle::* member;
};

constexpr FloatField kFloatFields[] = {
    {"Alpha", &ImGuiStyle::Alpha},
    {"DisabledAlpha", &ImGuiStyle::DisabledAlpha},
    {"WindowRounding", &ImGuiStyle::WindowRounding},
    {"WindowBorderSize", &ImGuiStyle::WindowBorderSize},
    {"ChildRounding", &ImGuiStyle::ChildRounding},
    {"ChildBorderSize", &ImGuiStyle::ChildBorderSize},
    {"PopupRounding", &ImGuiStyle::PopupRounding},
    {"PopupBorderSize", &ImGuiStyle::PopupBorderSize},
    {"FrameRounding", &ImGuiStyle::FrameRounding},
    {"FrameBorderSize", &ImGuiStyle::FrameBorderSize},
    {"IndentSpacing", &ImGuiStyle::IndentSpacing},
    {"ColumnsMinSpacing", &ImGuiStyle::ColumnsMinSpacing},
    {"ScrollbarSize", &ImGuiStyle::ScrollbarSize},
    {"ScrollbarRounding", &ImGuiStyle::ScrollbarRounding},
    {"ScrollbarPadding", &ImGuiStyle::ScrollbarPadding},
    {"GrabMinSize", &ImGuiStyle::GrabMinSize},
    {"GrabRounding", &ImGuiStyle::GrabRounding},
    {"ImageBorderSize", &ImGuiStyle::ImageBorderSize},
    {"TabRounding", &ImGuiStyle::TabRounding},
    {"TabBorderSize", &ImGuiStyle::TabBorderSize},
    {"TabBarBorderSize", &ImGuiStyle::TabBarBorderSize},
    {"TabBarOverlineSize", &ImGuiStyle::TabBarOverlineSize},
    {"TreeLinesSize", &ImGuiStyle::TreeLinesSize},
    {"TreeLinesRounding", &ImGuiStyle::TreeLinesRounding},
    {"SeparatorTextBorderSize", &ImGuiStyle::SeparatorTextBorderSize},
    {"DockingSeparatorSize", &ImGuiStyle::DockingSeparatorSize},
};

constexpr Vec2Field kVec2Fields[] = {
    {"WindowPadding", &ImGuiStyle::WindowPadding},
    {"WindowMinSize", &ImGuiStyle::WindowMinSize},
    {"WindowTitleAlign", &ImGuiStyle::WindowTitleAlign},
    {"FramePadding", &ImGuiStyle::FramePadding},
    {"ItemSpacing", &ImGuiStyle::ItemSpacing},
    {"ItemInnerSpacing", &ImGuiStyle::ItemInnerSpacing},
    {"CellPadding", &ImGuiStyle::CellPadding},
    {"TouchExtraPadding", &ImGuiStyle::TouchExtraPadding},
    {"ButtonTextAlign", &ImGuiStyle::ButtonTextAlign},
    {"SelectableTextAlign", &ImGuiStyle::SelectableTextAlign},
    {"SeparatorTextAlign", &ImGuiStyle::SeparatorTextAlign},
    {"SeparatorTextPadding", &ImGuiStyle::SeparatorTextPadding},
    {"DisplaySafeAreaPadding", &ImGuiStyle::DisplaySafeAreaPadding},
};

bool ParseFloat(const std::string& text, float& out) {
    if (text.empty()) return false;
    char* end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (end != text.c_str() + text.size() || !std::isfinite(value)) return false;
    out = value;
    return true;
}

int HexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool ParseHexColor(const std::string& text, ImVec4& out) {
    if ((text.size() != 7 && text.size() != 9) || text[0] != '#') return false;
    int channel[4] = {0, 0, 0, 255};
    for (std::size_t i = 0; i < (text.size() - 1) / 2; ++i) {
        const int high = HexDigit(text[1 + i * 2]);
        const int low  = HexDigit(text[2 + i * 2]);
        if (high < 0 || low < 0) return false;
        channel[i] = high * 16 + low;
    }
    out = ImVec4(static_cast<float>(channel[0]) / 255.0f, static_cast<float>(channel[1]) / 255.0f,
                 static_cast<float>(channel[2]) / 255.0f, static_cast<float>(channel[3]) / 255.0f);
    return true;
}

using Palette = std::unordered_map<std::string, ImVec4>;

// Grammar: ( '#RRGGBB' | '#RRGGBBAA' | '$name' ) [ '@' alpha ]
bool ParseColor(const std::string& text, const Palette& palette, ImVec4& out, std::string& error) {
    std::string body = text;
    float alphaScale = 1.0f;

    const std::size_t at = text.find('@');
    if (at != std::string::npos) {
        body = Trim(text.substr(0, at));
        const std::string alpha = Trim(text.substr(at + 1));
        if (!ParseFloat(alpha, alphaScale)) {
            error = "invalid alpha multiplier '" + alpha + "'";
            return false;
        }
        alphaScale = alphaScale < 0.0f ? 0.0f : (alphaScale > 1.0f ? 1.0f : alphaScale);
    }

    if (!body.empty() && body[0] == '$') {
        const auto it = palette.find(ToLower(body.substr(1)));
        if (it == palette.end()) {
            error = "unknown palette entry '" + body + "'";
            return false;
        }
        out = it->second;
    } else if (!ParseHexColor(body, out)) {
        error = "invalid color '" + body + "' (expected #RRGGBB, #RRGGBBAA or $name)";
        return false;
    }
    out.w *= alphaScale;
    return true;
}

bool ParseVec2(const std::string& text, ImVec2& out) {
    const std::size_t comma = text.find(',');
    if (comma == std::string::npos) return false;
    float x = 0.0f, y = 0.0f;
    if (!ParseFloat(Trim(text.substr(0, comma)), x) || !ParseFloat(Trim(text.substr(comma + 1)), y)) return false;
    out = ImVec2(x, y);
    return true;
}

std::string FormatHex(const ImVec4& color) {
    const auto toByte = [](float v) {
        const float clamped = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        return static_cast<unsigned>(std::lround(clamped * 255.0f));
    };
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "#%02X%02X%02X%02X", toByte(color.x), toByte(color.y), toByte(color.z),
                  toByte(color.w));
    return buffer;
}

}  // namespace

void Theme::SetBase(ThemeBase base) {
    *this = Theme();
    base_ = base;
}

bool Theme::LoadFromFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::ostringstream contents;
    contents << file.rdbuf();

    // Warnings are prefixed with the file name only, which keeps them short in the UI.
    const std::size_t slash = path.find_last_of("/\\");
    LoadFromString(contents.str(), slash == std::string::npos ? path : path.substr(slash + 1));
    return true;
}

void Theme::LoadFromString(const std::string& text, const std::string& sourceName) {
    *this = Theme();

    const auto warn = [&](int line, const std::string& message) {
        warnings_.push_back(sourceName + ":" + std::to_string(line) + ": " + message);
    };

    const detail::IniDocument doc = detail::ParseIni(text);
    for (const detail::IniError& error : doc.errors) warn(error.line, error.message);

    // Sections are processed by kind, not file order, so [palette] may appear after [colors].
    const auto forEachEntry = [&](const char* sectionName, const auto& handler) {
        for (const detail::IniSection& section : doc.sections) {
            if (ToLower(section.name) != sectionName) continue;
            for (const detail::IniEntry& entry : section.entries) handler(entry);
        }
    };

    forEachEntry("theme", [&](const detail::IniEntry& entry) {
        const std::string key = ToLower(entry.key);
        if (key == "name") {
            name_ = entry.value;
        } else if (key == "base") {
            const std::string value = ToLower(entry.value);
            if (value == "dark")         base_ = ThemeBase::Dark;
            else if (value == "light")   base_ = ThemeBase::Light;
            else if (value == "classic") base_ = ThemeBase::Classic;
            else warn(entry.line, "unknown base '" + entry.value + "' (expected dark, light or classic)");
        } else if (key != "author" && key != "description") {
            warn(entry.line, "unknown key '" + entry.key + "' in [theme]");
        }
    });

    Palette palette;
    forEachEntry("palette", [&](const detail::IniEntry& entry) {
        ImVec4 color;
        if (!ParseHexColor(entry.value, color)) {
            warn(entry.line, "palette entry '" + entry.key + "' must be #RRGGBB or #RRGGBBAA");
            return;
        }
        palette[ToLower(entry.key)] = color;
    });

    std::unordered_map<std::string, int> colorIndexByName;
    for (int i = 0; i < ImGuiCol_COUNT; ++i) colorIndexByName[ToLower(ImGui::GetStyleColorName(i))] = i;

    forEachEntry("colors", [&](const detail::IniEntry& entry) {
        const auto it = colorIndexByName.find(ToLower(entry.key));
        if (it == colorIndexByName.end()) {
            warn(entry.line, "unknown color '" + entry.key + "'");
            return;
        }
        ImVec4 color;
        std::string error;
        if (!ParseColor(entry.value, palette, color, error)) {
            warn(entry.line, error);
            return;
        }
        colors_[static_cast<std::size_t>(it->second)] = color;
    });

    forEachEntry("style", [&](const detail::IniEntry& entry) {
        const std::string key = ToLower(entry.key);
        for (const FloatField& field : kFloatFields) {
            if (key != ToLower(field.name)) continue;
            float value = 0.0f;
            if (ParseFloat(entry.value, value)) floats_.emplace_back(field.member, value);
            else warn(entry.line, "'" + entry.key + "' expects a number, got '" + entry.value + "'");
            return;
        }
        for (const Vec2Field& field : kVec2Fields) {
            if (key != ToLower(field.name)) continue;
            ImVec2 value;
            if (ParseVec2(entry.value, value)) vec2s_.emplace_back(field.member, value);
            else warn(entry.line, "'" + entry.key + "' expects 'x, y', got '" + entry.value + "'");
            return;
        }
        warn(entry.line, "unknown style field '" + entry.key + "'");
    });

    for (const detail::IniSection& section : doc.sections) {
        const std::string name = ToLower(section.name);
        if (name != "theme" && name != "palette" && name != "colors" && name != "style") {
            warn(section.line, section.name.empty() ? "entries outside of any section"
                                                    : "unknown section [" + section.name + "]");
        }
    }
}

void Theme::Apply(float dpiScale) const {
    ImGuiStyle& style = ImGui::GetStyle();

    // The base font size is owned by the font setup, not by themes.
    const float fontSizeBase = style.FontSizeBase;
    style = ImGuiStyle();
    style.FontSizeBase = fontSizeBase;

    switch (base_) {
        case ThemeBase::Light:   ImGui::StyleColorsLight(&style);   break;
        case ThemeBase::Classic: ImGui::StyleColorsClassic(&style); break;
        case ThemeBase::Dark:    ImGui::StyleColorsDark(&style);    break;
    }

    for (std::size_t i = 0; i < colors_.size(); ++i) {
        if (colors_[i]) style.Colors[i] = *colors_[i];
    }
    for (const auto& [member, value] : floats_) style.*member = value;
    for (const auto& [member, value] : vec2s_) style.*member = value;

    // Theme sizes are authored in logical pixels; scale them last.
    style.ScaleAllSizes(dpiScale);
    style.FontScaleDpi = dpiScale;
}

std::string Theme::Serialize(const ImGuiStyle& style, const std::string& name) {
    std::ostringstream out;
    out << "[theme]\nname = " << name << "\nbase = dark\n\n[colors]\n";
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        out << ImGui::GetStyleColorName(i) << " = " << FormatHex(style.Colors[i]) << "\n";
    }

    out << "\n[style]\n";
    char buffer[64];
    for (const FloatField& field : kFloatFields) {
        std::snprintf(buffer, sizeof buffer, "%g", static_cast<double>(style.*field.member));
        out << field.name << " = " << buffer << "\n";
    }
    for (const Vec2Field& field : kVec2Fields) {
        std::snprintf(buffer, sizeof buffer, "%g, %g", static_cast<double>((style.*field.member).x),
                      static_cast<double>((style.*field.member).y));
        out << field.name << " = " << buffer << "\n";
    }
    return out.str();
}

}  // namespace gui
