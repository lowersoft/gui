#include "theme_editor.h"

#include <algorithm>
#include <cfloat>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "imgui_stdlib.h"
#include "save_dialog.h"

namespace {

// With AltGr keyboard layouts Ctrl+Alt+S would also type a character; swallow it so the shortcut
int RejectSaveShortcutChar(ImGuiInputTextCallbackData*) {
    const ImGuiIO& io = ImGui::GetIO();
    return (io.KeyCtrl && io.KeyAlt && ImGui::IsKeyDown(ImGuiKey_S)) ? 1 : 0;
}

bool SaveShortcutPressed() {
    const ImGuiIO& io = ImGui::GetIO();
    return io.KeyCtrl && io.KeyAlt && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S, false);
}

}  // namespace

void ThemeEditor::Toggle() {
    if (open_) Close();
    else       Open();
}

void ThemeEditor::Open() {
    open_ = true;
    ClearStatus();
    LoadFromDisk(app_.ThemeFile());
    loadedRevision_ = app_.ThemeRevision();
}

void ThemeEditor::Close() {
    if (!open_) return;
    // The preview is only meaningful while the editor is open, so unsaved edits are dropped.
    if (dirty_) app_.ReloadTheme();
    open_  = false;
    dirty_ = false;
}

void ThemeEditor::Discard() {
    dirty_ = false;
}

void ThemeEditor::LoadFromDisk(const std::string& file) {
    loadedFile_ = file;
    dirty_      = false;
    text_.clear();
    savedText_.clear();

    if (file.empty()) return;

    std::ifstream in(file, std::ios::binary);
    if (!in) {
        status_        = "Cannot read " + file;
        statusIsError_ = true;
        return;
    }
    std::ostringstream contents;
    contents << in.rdbuf();
    text_ = contents.str();

    // The editor works with '\n' only; files are written back with '\n'.
    text_.erase(std::remove(text_.begin(), text_.end(), '\r'), text_.end());
    savedText_ = text_;
}

// Keeps the buffer consistent when the app switches themes or the file changes on disk.
void ThemeEditor::SyncWithApp() {
    const bool fileChanged     = app_.ThemeFile() != loadedFile_;
    const bool revisionChanged = app_.ThemeRevision() != loadedRevision_;
    loadedRevision_            = app_.ThemeRevision();

    if (fileChanged || (revisionChanged && !dirty_)) {
        // A reload of the same file (including our own save) keeps the status line; a new file clears it.
        if (fileChanged && !keepStatus_) ClearStatus();
        keepStatus_ = false;
        LoadFromDisk(app_.ThemeFile());
    } else if (revisionChanged && dirty_) {
        // The file was reloaded underneath unsaved edits; show the edits again.
        app_.PreviewThemeText(text_);
    }
}

void ThemeEditor::ClearStatus() {
    status_.clear();
    statusIsError_ = false;
}

bool ThemeEditor::Save() {
    std::string error;
    if (!WriteTextFile(loadedFile_, text_, error)) {
        status_        = "Save failed: " + error;
        statusIsError_ = true;
        return false;
    }

    savedText_     = text_;
    dirty_         = false;
    status_        = "Saved " + std::filesystem::path(loadedFile_).filename().string();
    statusIsError_ = false;
    return true;
}

// Leaves the original file untouched and continues editing the copy.
bool ThemeEditor::SaveCopy(const std::string& path) {
    std::string error;
    if (!WriteTextFile(path, text_, error)) {
        status_        = "Save failed: " + error;
        statusIsError_ = true;
        return false;
    }

    status_        = "Saved copy " + std::filesystem::path(path).filename().string();
    statusIsError_ = false;
    keepStatus_    = true;
    app_.SetTheme(path);
    return true;
}

void ThemeEditor::Draw() {
    if (!open_) return;

    SyncWithApp();

    const float fontSize = ImGui::GetFontSize();
    // Opens to the right of the default position of the "Hello" window instead of covering it.
    ImGui::SetNextWindowPos(ImVec2(fontSize * 24.0f, 60.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(fontSize * 36.0f, fontSize * 32.0f), ImGuiCond_FirstUseEver);

    bool stillOpen = true;
    const bool visible = ImGui::Begin("Theme Text Editor", &stillOpen);
    if (visible) {
        if (loadedFile_.empty()) {
            ImGui::TextWrapped("No theme file is loaded, so there is nothing to edit.");
        } else {
            ImGui::TextDisabled("%s%s", loadedFile_.c_str(), dirty_ ? "  (unsaved changes)" : "");

            const ImGuiStyle& style = ImGui::GetStyle();
            const auto& warnings    = app_.CurrentTheme().Warnings();
            const float lineHeight  = ImGui::GetTextLineHeightWithSpacing();
            const float warningsHeight =
                lineHeight * static_cast<float>(std::min<std::size_t>(std::max<std::size_t>(warnings.size(), 1), 4)) +
                style.WindowPadding.y * 2.0f;
            const float footerHeight = warningsHeight + ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y;

            const ImGuiInputTextFlags flags =
                ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackCharFilter;
            if (ImGui::InputTextMultiline("##theme_text", &text_, ImVec2(-FLT_MIN, -footerHeight), flags,
                                          RejectSaveShortcutChar)) {
                dirty_ = (text_ != savedText_);
                ClearStatus();
                app_.PreviewThemeText(text_);
            }

            if (ImGui::BeginChild("##theme_warnings", ImVec2(0.0f, warningsHeight), ImGuiChildFlags_Borders)) {
                if (warnings.empty()) {
                    ImGui::TextDisabled("No warnings");
                }
                for (const std::string& warning : warnings) {
                    ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "%s", warning.c_str());
                }
            }
            ImGui::EndChild();

            DrawFooter();
        }
    }
    ImGui::End();

    if (!stillOpen) Close();
}

void ThemeEditor::DrawFooter() {
    bool requestSave = ImGui::Button("Save (Ctrl+Alt+S)");
    ImGui::SameLine();
    if (ImGui::Button("Revert")) {
        ClearStatus();
        LoadFromDisk(loadedFile_);
        app_.ReloadTheme();
    }
    if (!status_.empty()) {
        ImGui::SameLine();
        if (statusIsError_) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", status_.c_str());
        else                ImGui::TextDisabled("%s", status_.c_str());
    }

    if ((requestSave || SaveShortcutPressed()) && !saveDialog_.IsActive()) saveDialog_.Request();

    switch (saveDialog_.Draw(loadedFile_)) {
        case SaveDialog::Result::Overwrite: Save(); break;
        case SaveDialog::Result::SaveCopy:  SaveCopy(saveDialog_.CopyPath()); break;
        case SaveDialog::Result::None:      break;
    }
}
