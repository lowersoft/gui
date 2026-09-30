/// @file theme.h
/// @brief INI-driven Dear ImGui theme (colors and style metrics).
///
/// File layout (see conf/theme.ini for a complete example):
/// @code
/// [theme]     name, base (dark | light | classic), author, description
/// [palette]   name = #RRGGBB[AA]        named colors, referenced as $name
/// [colors]    <ImGuiCol name> = <color>  e.g. WindowBg = $bg, Button = $accent @0.6
/// [style]     <ImGuiStyle field> = <number> | <x>, <y>
/// @endcode
/// A color is `#RRGGBB`, `#RRGGBBAA` or `$palette_name`, optionally followed by `@alpha`
/// which multiplies the resulting alpha. Keys are case-insensitive. Anything not listed in
/// the file keeps the value of the selected base theme.
#pragma once

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "imgui.h"

namespace gui {

/// Built-in ImGui color scheme used as the starting point of a theme.
enum class ThemeBase { Dark, Light, Classic };

class Theme {
public:
    /// Loads a theme from disk.
    /// @return false if the file could not be read. Malformed lines never fail the load;
    ///         they are skipped and reported through Warnings().
    bool LoadFromFile(const std::string& path);

    /// Parses theme text. @p sourceName only prefixes warning messages.
    void LoadFromString(const std::string& text, const std::string& sourceName = "<memory>");

    /// Writes the theme into the current ImGui style, replacing it entirely.
    /// Must be called with a live ImGui context and outside of an active frame.
    /// @param dpiScale Applied to all sizes and to font scaling after the theme values.
    void Apply(float dpiScale = 1.0f) const;

    /// Clears all overrides and selects a built-in base.
    void SetBase(ThemeBase base);

    const std::string&              Name() const { return name_; }
    ThemeBase                       Base() const { return base_; }
    const std::vector<std::string>& Warnings() const { return warnings_; }

    /// Serialises a complete style (every color and supported metric) to theme INI text.
    static std::string Serialize(const ImGuiStyle& style, const std::string& name = "Exported");

private:
    std::string name_ = "Unnamed";
    ThemeBase   base_ = ThemeBase::Dark;

    std::array<std::optional<ImVec4>, ImGuiCol_COUNT>   colors_;
    std::vector<std::pair<float ImGuiStyle::*, float>>  floats_;
    std::vector<std::pair<ImVec2 ImGuiStyle::*, ImVec2>> vec2s_;
    std::vector<std::string>                            warnings_;
};

}  // namespace gui
