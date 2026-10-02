/// @file theme_editor.h
/// @brief In-app editor for the active theme's INI file.
///
/// Edits are previewed live. Saving asks for confirmation and either overwrites the theme file or
/// writes a copy; it can be triggered with the Save button or Ctrl+Alt+S.
#pragma once

#include <string>

#include "gui/app.h"
#include "save_dialog.h"

class ThemeEditor {
public:
    explicit ThemeEditor(gui::App& app) : app_(app) {}

    void Toggle();
    void Close();
    bool IsOpen() const { return open_; }

    /// Drops unsaved edits so the editor shows the file on disk again.
    void Discard();

    /// Call once per frame from App::OnGui().
    void Draw();

private:
    void Open();
    void SyncWithApp();
    void LoadFromDisk(const std::string& file);
    void ClearStatus();
    bool Save();
    bool SaveCopy(const std::string& path);
    void DrawFooter();

    gui::App& app_;

    bool open_  = false;
    bool dirty_ = false;
    bool keepStatus_ = false;  ///< Keeps the status line across the theme switch that follows a copy.

    SaveDialog saveDialog_;

    std::string text_;           ///< Buffer shown in the editor.
    std::string savedText_;      ///< Content of the file as last read or written.
    std::string loadedFile_;     ///< Theme file the buffer belongs to.
    unsigned    loadedRevision_ = 0;

    std::string status_;
    bool        statusIsError_ = false;
};
