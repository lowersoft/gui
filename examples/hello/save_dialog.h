/// @file save_dialog.h
/// @brief Shared confirmation popup for saving a theme file, plus the file helpers it relies on.
#pragma once

#include <string>

/// Writes @p text to @p target through a temporary file so a failure never leaves a truncated file.
/// @param error Receives a human readable reason when the function returns false.
bool WriteTextFile(const std::string& target, const std::string& text, std::string& error);

/// Modal popup offering "Overwrite", "Save as copy" and "Cancel".
///
/// "Save as copy" asks for a file name (without extension) that must not exist yet. The dialog only
/// reports the user's choice; writing the file is up to the caller.
class SaveDialog {
public:
    enum class Result { None, Overwrite, SaveCopy };

    /// Opens the popup on the next Draw() call.
    void Request() { openRequested_ = true; }

    /// True while the popup is visible or about to be.
    bool IsActive() const { return openRequested_ || active_; }

    /// Call every frame from the window that requested the dialog.
    /// @param target Theme file that would be overwritten; copies are created next to it.
    /// @param note   Optional extra line shown in the popup (e.g. what the save will change).
    Result Draw(const std::string& target, const char* note = nullptr);

    /// Full path chosen by the last Result::SaveCopy.
    const std::string& CopyPath() const { return copyPath_; }

private:
    bool ValidateCopyName(const std::string& target);

    bool        openRequested_ = false;
    bool        active_        = false;
    std::string copyName_;
    std::string copyPath_;
    std::string error_;
};
