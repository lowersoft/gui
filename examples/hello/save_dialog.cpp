#include "save_dialog.h"

#include <filesystem>
#include <fstream>

#include "imgui.h"
#include "imgui_stdlib.h"

namespace fs = std::filesystem;

namespace {

constexpr const char* kPopupId = "Overwrite theme file?";

// First "<stem>-copy", "<stem>-copy2", ... that does not exist next to the target.
std::string SuggestCopyName(const fs::path& target) {
    const std::string stem = target.stem().string();
    std::string       name = stem + "-copy";
    for (int n = 2; n < 1000; ++n) {
        std::error_code ec;
        if (!fs::exists(target.parent_path() / (name + ".ini"), ec)) break;
        name = stem + "-copy" + std::to_string(n);
    }
    return name;
}

bool HasForbiddenCharacter(const std::string& name) {
    return name.find_first_of("\\/:*?\"<>|") != std::string::npos;
}

}  // namespace

bool WriteTextFile(const std::string& target, const std::string& text, std::string& error) {
    fs::path temporary = target;
    temporary += ".tmp";

    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        out << text;
        out.flush();
        if (!out) {
            error = "cannot write " + temporary.string();
            return false;
        }
    }

    std::error_code ec;
    fs::rename(temporary, target, ec);
    if (ec) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        error = ec.message();
        return false;
    }
    return true;
}

bool SaveDialog::ValidateCopyName(const std::string& target) {
    std::string name = copyName_;
    // Accept "name.ini" as well as "name".
    if (name.size() > 4 && name.compare(name.size() - 4, 4, ".ini") == 0) name.resize(name.size() - 4);

    if (name.empty() || name == "." || name == "..") {
        error_ = "Enter a file name.";
        return false;
    }
    if (HasForbiddenCharacter(name)) {
        error_ = "The name contains characters that are not allowed.";
        return false;
    }

    const fs::path path = fs::path(target).parent_path() / (name + ".ini");
    std::error_code ec;
    if (fs::exists(path, ec)) {
        error_ = path.filename().string() + " already exists.";
        return false;
    }

    copyPath_ = path.string();
    error_.clear();
    return true;
}

SaveDialog::Result SaveDialog::Draw(const std::string& target, const char* note) {
    if (openRequested_) {
        openRequested_ = false;
        copyName_      = SuggestCopyName(target);
        error_.clear();
        ImGui::OpenPopup(kPopupId);
    }

    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    active_ = ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    if (!active_) return Result::None;

    Result      result = Result::None;
    const float font   = ImGui::GetFontSize();

    ImGui::Text("Are you sure you want to overwrite this theme file?");
    ImGui::TextDisabled("%s", target.c_str());
    if (note != nullptr) ImGui::TextDisabled("%s", note);
    ImGui::Spacing();

    ImGui::TextUnformatted("Or keep the original and save a copy named:");
    ImGui::SetNextItemWidth(font * 18.0f);
    bool copyRequested = ImGui::InputText("##copy_name", &copyName_, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    ImGui::TextDisabled(".ini");
    if (!error_.empty()) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", error_.c_str());
    ImGui::Spacing();

    const ImVec2 size(font * 8.0f, 0.0f);
    if (ImGui::Button("Overwrite", size)) {
        result = Result::Overwrite;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Save as copy", size)) copyRequested = true;
    ImGui::SameLine();
    if (ImGui::Button("Cancel", size)) ImGui::CloseCurrentPopup();
    // Cancel is the default so an accidental Enter never overwrites the file.
    ImGui::SetItemDefaultFocus();

    if (copyRequested && ValidateCopyName(target)) {
        result = Result::SaveCopy;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    return result;
}
