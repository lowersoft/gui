#include "style_editor.h"

#include <algorithm>
#include <filesystem>

#include "gui/theme.h"
#include "imgui.h"

namespace {

bool SaveShortcutPressed() {
    const ImGuiIO& io = ImGui::GetIO();
    return io.KeyCtrl && io.KeyAlt && !io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S, false);
}

}  // namespace

void StyleEditor::Toggle() {
    if (open_) Close();
    else       Open();
}

void StyleEditor::Open() {
    open_           = true;
    status_.clear();
    loadedRevision_ = app_.ThemeRevision();
    baseline_       = CurrentText();
    dirty_          = false;
}

void StyleEditor::Close() {
    if (!open_) return;
    // Edits only live in the ImGui style, so reloading the file is enough to drop them.
    if (dirty_) app_.ReloadTheme();
    open_  = false;
    dirty_ = false;
}

std::string StyleEditor::CurrentText() const {
    const gui::Theme& theme = app_.CurrentTheme();
    return gui::Theme::Serialize(ImGui::GetStyle(), theme.Name(), theme.Base(), app_.DpiScale());
}

bool StyleEditor::Save(const std::string& path, bool isCopy) {
    std::string error;
    if (!WriteTextFile(path, CurrentText(), error)) {
        status_        = "Save failed: " + error;
        statusIsError_ = true;
        return false;
    }

    const std::string name = std::filesystem::path(path).filename().string();
    status_                = (isCopy ? "Saved copy " : "Saved ") + name;
    statusIsError_         = false;
    baseline_              = CurrentText();
    // Switching to the copy makes the editor keep working on the file that now holds the edits.
    if (isCopy) app_.SetTheme(path);
    return true;
}

void StyleEditor::Draw() {
    if (!open_) return;

    // A theme load (switch, reload, hot reload) replaces the style, so edits made before it are gone.
    if (app_.ThemeRevision() != loadedRevision_) {
        loadedRevision_ = app_.ThemeRevision();
        baseline_       = CurrentText();
    }

    const float fontSize = ImGui::GetFontSize();
    ImGui::SetNextWindowPos(ImVec2(fontSize * 24.0f, 60.0f), ImGuiCond_FirstUseEver);
    const float maxHeight = ImGui::GetMainViewport()->Size.y - 100.0f;
    ImGui::SetNextWindowSize(ImVec2(fontSize * 36.0f, std::min(fontSize * 40.0f, maxHeight)), ImGuiCond_FirstUseEver);

    bool stillOpen = true;
    if (ImGui::Begin("Dear ImGui Style Editor", &stillOpen)) {
        dirty_ = CurrentText() != baseline_;

        const std::string& file = app_.ThemeFile();
        if (file.empty()) {
            ImGui::TextDisabled("No theme file is loaded; saving is unavailable.");
        } else {
            ImGui::TextDisabled("%s%s", file.c_str(), dirty_ ? "  (unsaved changes)" : "");
        }

        const ImGuiStyle& style = ImGui::GetStyle();
        const float footer      = ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y;
        if (ImGui::BeginChild("##style_editor_body", ImVec2(0.0f, -footer))) {
            ImGui::ShowStyleEditor();
        }
        ImGui::EndChild();

        DrawFooter();
    }
    ImGui::End();

    if (!stillOpen) Close();
}

void StyleEditor::DrawFooter() {
    const bool canSave = !app_.ThemeFile().empty();

    ImGui::BeginDisabled(!canSave);
    const bool requestSave = ImGui::Button("Save (Ctrl+Alt+S)");
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Revert")) {
        status_.clear();
        app_.ReloadTheme();
    }
    if (!status_.empty()) {
        ImGui::SameLine();
        if (statusIsError_) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", status_.c_str());
        else                ImGui::TextDisabled("%s", status_.c_str());
    }

    const bool shortcut = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && SaveShortcutPressed();
    if (canSave && (requestSave || shortcut) && !saveDialog_.IsActive()) saveDialog_.Request();

    static const char* kNote = "Comments and palette references in the file are replaced by explicit values.";
    switch (saveDialog_.Draw(app_.ThemeFile(), kNote)) {
        case SaveDialog::Result::Overwrite: Save(app_.ThemeFile(), false); break;
        case SaveDialog::Result::SaveCopy:  Save(saveDialog_.CopyPath(), true); break;
        case SaveDialog::Result::None:      break;
    }
}
