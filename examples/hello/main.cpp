#include <array>
#include <cstdio>

#include "gui/app.h"
#include "style_editor.h"
#include "theme_editor.h"

class HelloApp : public gui::App {
public:
    void OnInit() override { std::puts("HelloApp initialised"); }

    void OnGui() override {
        ImGui::Begin("Hello", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Unicode glyph test: \xC3\xA9\xC3\xB1\xC3\xBC \xC5\x9F\xC4\x9F\xC4\xB1");
        ImGui::Text("FPS: %.1f", static_cast<double>(ImGui::GetIO().Framerate));
        ImGui::Separator();

        if (ImGui::Button("Click")) ++counter_;
        ImGui::SameLine();
        ImGui::Text("Counter: %d", counter_);

        ImGui::SliderFloat("Speed", &speed_, 0.0f, 10.0f);
        ImGui::ColorEdit3("Color", &color_.x);
        ImGui::Checkbox("Show ImGui demo window", &showDemo_);

        ImGui::SeparatorText("Theme");
        ImGui::Text("Active: %s", CurrentTheme().Name().c_str());
        if (ImGui::Button("Next theme")) {
            themeIndex_ = (themeIndex_ + 1) % kThemes.size();
            SetTheme(kThemes[themeIndex_]);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload")) {
            textEditor_.Discard();
            ReloadTheme();
        }

        // Both editors change the live style, so only one of them is open at a time.
        if (ImGui::Button("Theme editor")) {
            textEditor_.Close();
            styleEditor_.Toggle();
        }
        ImGui::SameLine();
        if (ImGui::Button("Text editor")) {
            styleEditor_.Close();
            textEditor_.Toggle();
        }

        for (const std::string& warning : CurrentTheme().Warnings()) {
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "%s", warning.c_str());
        }

        ImGui::Separator();
        if (ImGui::Button("Quit")) RequestClose();
        ImGui::End();

        styleEditor_.Draw();
        textEditor_.Draw();

        if (showDemo_) ImGui::ShowDemoWindow(&showDemo_);
    }

private:
    static constexpr std::array<const char*, 5> kThemes = {
        "conf/theme.ini", "conf/themes/light.ini", "conf/themes/forest.ini",
        "conf/themes/crimson.ini", "conf/themes/classic.ini"};

    StyleEditor styleEditor_{*this};
    ThemeEditor textEditor_{*this};

    bool        showDemo_   = false;
    int         counter_    = 0;
    std::size_t themeIndex_ = 0;
    float       speed_      = 1.0f;
    ImVec4      color_      = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
};

GUI_MAIN(HelloApp, "ImGui Hello", 1280, 720)
