/// @file main.cpp
/// @brief File transfer example: a fixed-size window with a drop-in file list and a send bar.
///
/// Files and folders are added with the native file dialogs (File menu) or by dropping them onto the
/// window. Sending is a placeholder; the example demonstrates layout, theming and input handling with
/// the framework.
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <future>
#include <string>
#include <vector>

#include "gui/app.h"
#include "gui/file_dialog.h"
#include "imgui.h"
#include "imgui_stdlib.h"

namespace fs = std::filesystem;

namespace {

struct TransferItem {
    fs::path      path;
    std::uint64_t bytes    = 0;
    bool          isFolder = false;
    bool          partial  = false;  ///< The size is a lower bound because the folder scan was capped.
    bool          selected = false;
};

constexpr std::size_t kMaxScannedEntries = 100000;

std::string FormatBytes(std::uint64_t bytes, bool partial) {
    static constexpr std::array<const char*, 5> kUnits = {"B", "KB", "MB", "GB", "TB"};

    double      value = static_cast<double>(bytes);
    std::size_t unit  = 0;
    while (value >= 1024.0 && unit + 1 < kUnits.size()) {
        value /= 1024.0;
        ++unit;
    }

    char buffer[48];
    if (unit == 0) std::snprintf(buffer, sizeof buffer, "%s%llu %s", partial ? ">" : "",
                                 static_cast<unsigned long long>(bytes), kUnits[unit]);
    else           std::snprintf(buffer, sizeof buffer, "%s%.1f %s", partial ? ">" : "", value, kUnits[unit]);
    return buffer;
}

/// Reads size information for a dropped path. Returns false if it is neither a file nor a folder.
bool Inspect(const fs::path& path, TransferItem& item) {
    std::error_code ec;
    item.path = path;

    if (fs::is_directory(path, ec)) {
        item.isFolder = true;
        std::size_t scanned = 0;
        for (fs::recursive_directory_iterator it(path, fs::directory_options::skip_permission_denied, ec), end;
             !ec && it != end; it.increment(ec)) {
            if (++scanned > kMaxScannedEntries) {
                item.partial = true;
                break;
            }
            std::error_code sizeError;
            if (it->is_regular_file(sizeError)) {
                const std::uintmax_t size = it->file_size(sizeError);
                if (!sizeError) item.bytes += size;
            }
        }
        return true;
    }

    if (fs::is_regular_file(path, ec)) {
        const std::uintmax_t size = fs::file_size(path, ec);
        item.bytes                = ec ? 0 : size;
        return true;
    }
    return false;
}

}  // namespace

class FileTransferApp : public gui::App {
public:
    void OnInit() override {
        GLFWwindow* window = Window();
        if (window == nullptr) return;

        // The layout is designed for one window size.
        glfwSetWindowAttrib(window, GLFW_RESIZABLE, GLFW_FALSE);
        glfwSetWindowUserPointer(window, this);
        glfwSetDropCallback(window, [](GLFWwindow* w, int count, const char** paths) {
            auto* self = static_cast<FileTransferApp*>(glfwGetWindowUserPointer(w));
            for (int i = 0; i < count; ++i) self->AddPath(fs::u8path(paths[i]));
        });
    }

    void OnGui() override {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        constexpr ImGuiWindowFlags kFlags =
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

        ImGui::Begin("File Transfer", nullptr, kFlags);

        CollectDialogResult();
        HandleShortcuts();
        DrawMenuBar();
        DrawFileList();
        DrawSendBar();
        DrawPopups();

        ImGui::End();
    }

private:
    struct ThemeChoice {
        const char* label;
        const char* path;
    };

    static constexpr std::array<ThemeChoice, 5> kThemes = {{
        {"Default", "conf/theme.ini"},
        {"Daylight", "conf/themes/light.ini"},
        {"Forest", "conf/themes/forest.ini"},
        {"Crimson", "conf/themes/crimson.ini"},
        {"Classic", "conf/themes/classic.ini"},
    }};

    // Actions shared by menu items and keyboard shortcuts.
    void AddPath(const fs::path& path) {
        const bool known = std::any_of(items_.begin(), items_.end(),
                                       [&](const TransferItem& item) { return item.path == path; });
        if (known) return;

        TransferItem item;
        if (Inspect(path, item)) items_.push_back(std::move(item));
        else                     status_ = "Skipped " + path.filename().string() + " (not a file or folder)";
    }

    bool DialogOpen() const { return dialog_.valid(); }

    // The dialog blocks, so it runs on a worker thread while the UI keeps drawing.
    void OpenDialog(bool folders) {
        GLFWwindow* window = Window();
        dialog_            = std::async(std::launch::async, [window, folders] {
            return folders ? gui::PickFolders(window) : gui::PickFiles(window);
        });
    }

    void CollectDialogResult() {
        if (!dialog_.valid() || dialog_.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;

        const gui::FileDialogResult result = dialog_.get();
        if (!result.error.empty()) status_ = result.error;
        for (const std::string& path : result.paths) AddPath(fs::u8path(path));
    }

    void SelectAll(bool selected) {
        for (TransferItem& item : items_) item.selected = selected;
    }

    void RemoveSelected() {
        items_.erase(std::remove_if(items_.begin(), items_.end(), [](const TransferItem& i) { return i.selected; }),
                     items_.end());
    }

    void ClearAll() {
        items_.clear();
        status_.clear();
    }

    std::size_t SelectedCount() const {
        return static_cast<std::size_t>(std::count_if(items_.begin(), items_.end(),
                                                      [](const TransferItem& i) { return i.selected; }));
    }

    std::uint64_t SelectedBytes() const {
        std::uint64_t total = 0;
        for (const TransferItem& item : items_) total += item.selected ? item.bytes : 0;
        return total;
    }

    void HandleShortcuts() {
        // Text fields keep Delete and Ctrl+A for themselves.
        if (ImGui::GetIO().WantTextInput) return;

        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_A)) SelectAll(true);
        if (ImGui::Shortcut(ImGuiKey_Delete)) RemoveSelected();
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Delete)) ClearAll();
    }

    void DrawMenuBar() {
        if (!ImGui::BeginMenuBar()) return;

        if (ImGui::BeginMenu("File")) {
            const bool idle = !DialogOpen();
            if (ImGui::MenuItem("Add files...", nullptr, false, idle)) OpenDialog(false);
            if (ImGui::MenuItem("Add folder...", nullptr, false, idle)) OpenDialog(true);
            ImGui::Separator();
            if (ImGui::MenuItem("Remove selected", "Del", false, SelectedCount() > 0)) RemoveSelected();
            if (ImGui::MenuItem("Clear all", "Ctrl+Shift+Del", false, !items_.empty())) ClearAll();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) RequestClose();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Select all", "Ctrl+A", false, !items_.empty())) SelectAll(true);
            if (ImGui::MenuItem("Select none", nullptr, false, !items_.empty())) SelectAll(false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::BeginMenu("Theme")) {
                const std::string current = fs::path(ThemeFile()).filename().string();
                for (const ThemeChoice& choice : kThemes) {
                    const bool active = fs::path(choice.path).filename().string() == current;
                    if (ImGui::MenuItem(choice.label, nullptr, active)) SetTheme(choice.path);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) openAbout_ = true;
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    void DrawFileList() {
        const ImGuiStyle& style = ImGui::GetStyle();
        const float footerHeight =
            ImGui::GetTextLineHeightWithSpacing() + ImGui::GetFrameHeightWithSpacing() + style.ItemSpacing.y * 2.0f;

        if (items_.empty()) {
            DrawEmptyState(footerHeight);
            return;
        }

        constexpr ImGuiTableFlags kTableFlags = ImGuiTableFlags_ScrollY |
                                                ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_PadOuterX |
                                                ImGuiTableFlags_SizingFixedFit;
        if (!ImGui::BeginTable("##files", 3, kTableFlags, ImVec2(0.0f, -footerHeight))) return;

        const float checkboxWidth = ImGui::GetFrameHeight();
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("##select",
                                ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize |
                                    ImGuiTableColumnFlags_NoHeaderLabel,
                                checkboxWidth);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 6.0f);

        ImGui::TableHeadersRow();

        for (std::size_t i = 0; i < items_.size(); ++i) {
            TransferItem& item = items_[i];
            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Checkbox("##select", &item.selected);

            ImGui::TableSetColumnIndex(1);
            const std::string name = item.path.filename().string() + (item.isFolder ? "/" : "");
            if (ImGui::Selectable(name.c_str(), item.selected, ImGuiSelectableFlags_SpanAllColumns |
                                                                    ImGuiSelectableFlags_AllowOverlap)) {
                item.selected = !item.selected;
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
                ImGui::SetTooltip("%s", item.path.string().c_str());
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::TextDisabled("%s", FormatBytes(item.bytes, item.partial).c_str());
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    void DrawEmptyState(float footerHeight) {
        if (ImGui::BeginChild("##empty", ImVec2(0.0f, -footerHeight), ImGuiChildFlags_None)) {
            const char*  message = "Use File > Add files, or drop files here";
            const ImVec2 size    = ImGui::CalcTextSize(message);
            const ImVec2 avail   = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPos(ImVec2((avail.x - size.x) * 0.5f, ImGui::GetCursorPosY() + avail.y * 0.4f));
            ImGui::TextDisabled("%s", message);
        }
        ImGui::EndChild();
    }

    void DrawSendBar() {
        const std::size_t selected = SelectedCount();
        if (!status_.empty()) {
            ImGui::TextDisabled("%s", status_.c_str());
        } else if (items_.empty()) {
            ImGui::TextDisabled("No files added");
        } else {
            ImGui::TextDisabled("%zu of %zu selected (%s)", selected, items_.size(),
                                FormatBytes(SelectedBytes(), false).c_str());
        }

        const ImGuiStyle& style = ImGui::GetStyle();
        const float buttonWidth = ImGui::GetFontSize() * 5.0f;
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - buttonWidth - style.ItemSpacing.x);
        ImGui::InputTextWithHint("##peer", "IP or Peer-Code", &peer_);

        ImGui::SameLine();
        const bool canSend = selected > 0 && !peer_.empty();
        ImGui::BeginDisabled(!canSend);
        if (ImGui::Button("Send", ImVec2(buttonWidth, 0.0f))) {
            status_ = "Sending is not implemented in this example";
        }
        ImGui::EndDisabled();
    }

    void DrawPopups() {
        if (openAbout_) {
            ImGui::OpenPopup("About");
            openAbout_ = false;
        }

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextUnformatted("File Transfer example");
            ImGui::TextDisabled("Theme: %s", CurrentTheme().Name().c_str());
            ImGui::Spacing();
            if (ImGui::Button("Close", ImVec2(ImGui::GetFontSize() * 6.0f, 0.0f))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    std::vector<TransferItem> items_;
    std::string               peer_;
    std::string               status_;
    std::future<gui::FileDialogResult> dialog_;
    bool                      openAbout_   = false;
};

GUI_MAIN(FileTransferApp, "File Transfer", 520, 820)
