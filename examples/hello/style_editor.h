/// @file style_editor.h
/// @brief Dear ImGui's Style Editor wired to the active theme file.
///
/// Edits change the live ImGui style only. The theme file is written when the user saves, after a
/// confirmation popup that offers to overwrite the file or to save a copy instead.
#pragma once

#include <string>

#include "gui/app.h"
#include "save_dialog.h"

class StyleEditor {
public:
    explicit StyleEditor(gui::App& app) : app_(app) {}

    void Toggle();
    void Close();
    bool IsOpen() const { return open_; }

    /// Call once per frame from App::OnGui().
    void Draw();

private:
    void        Open();
    std::string CurrentText() const;
    bool        Save(const std::string& path, bool isCopy);
    void        DrawFooter();

    gui::App& app_;

    bool open_  = false;
    bool dirty_ = false;

    std::string baseline_;  ///< Serialised style of the theme as loaded; used to detect edits.
    unsigned    loadedRevision_ = 0;

    SaveDialog  saveDialog_;
    std::string status_;
    bool        statusIsError_ = false;
};
