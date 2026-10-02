/// @file app.h
/// @brief Thin application wrapper around Dear ImGui, GLFW and OpenGL 3.
///
/// @code
/// class MyApp : public gui::App {
///     void OnGui() override { ImGui::Text("Hello"); }
/// };
/// GUI_MAIN(MyApp, "Title", 1280, 720)
/// @endcode
#pragma once

#include <string>

#include "gui/theme.h"
#include "imgui.h"

struct GLFWwindow;

namespace gui {

/// Startup options passed to App::Run().
struct AppConfig {
    std::string title = "ImGui App";
    int  width  = 1280;   ///< Initial client width in logical pixels (scaled by monitor DPI).
    int  height = 720;    ///< Initial client height in logical pixels (scaled by monitor DPI).
    bool vsync  = true;

    bool docking    = true;   ///< Enable window docking.
    bool dockspace  = true;   ///< Cover the main viewport with a dockspace (requires docking).
    bool viewports  = false;  ///< Allow windows to be dragged out into native OS windows.
    bool saveLayout = true;   ///< Persist window layout to imgui.ini.

    /// Theme file, resolved via the source tree (Debug builds), the executable directory, then the working
    /// directory. Empty disables file-based theming.
    std::string themePath = "conf/theme.ini";
    bool themeHotReload   = true;   ///< Re-apply the theme whenever its file changes on disk.
    bool darkTheme        = true;   ///< Built-in fallback used when the theme file cannot be loaded.

    float fontSize = 18.0f;   ///< Base font size in pixels, before DPI scaling.
    /// Path to a TTF file. When empty, a common system font is used if one is found.
    std::string fontPath;

    ImVec4 clearColor = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
};

/// Base class for applications. Derive and override the hooks below.
class App {
public:
    App() = default;
    virtual ~App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /// Creates the window and runs the main loop until it is closed.
    /// @return Process exit code; 0 on success.
    int Run(const AppConfig& config = {});

    /// Asks the main loop to exit after the current frame.
    void RequestClose();

    /// Loads and applies a theme file at the start of the next frame (safe to call from OnGui()).
    /// A file that cannot be read leaves the current theme untouched.
    void SetTheme(const std::string& path);

    /// Re-reads the current theme file at the start of the next frame.
    void ReloadTheme();

    /// Applies theme text (for example from an in-app editor) at the start of the next frame.
    /// The theme file on disk is not touched; call ReloadTheme() to discard the preview.
    void PreviewThemeText(const std::string& text);

    /// The theme currently in effect, including any warnings from its last load.
    const Theme& CurrentTheme() const { return theme_; }

    /// Resolved path of the active theme file; empty when no file is loaded.
    const std::string& ThemeFile() const { return themeFile_; }

    /// Incremented every time a theme file is loaded from disk.
    unsigned ThemeRevision() const { return themeRevision_; }

    /// Scale applied to all theme sizes (monitor content scale at startup).
    float DpiScale() const { return dpiScale_; }

    GLFWwindow*      Window() { return window_; }
    const AppConfig& Config() const { return config_; }

protected:
    /// Called once after ImGui and the renderer are initialised.
    virtual void OnInit() {}

    /// Called every frame before OnGui().
    /// @param dt Seconds elapsed since the previous frame.
    virtual void OnUpdate(float dt) { (void)dt; }

    /// Called every frame between ImGui::NewFrame() and ImGui::Render().
    virtual void OnGui() = 0;

    /// Called once before ImGui and the window are destroyed.
    virtual void OnShutdown() {}

private:
    bool LoadTheme(const std::string& path);
    void ApplyTheme(const Theme& theme);
    void UpdateTheme();

    GLFWwindow* window_ = nullptr;
    AppConfig   config_;

    Theme       theme_;
    std::string themeFile_;            ///< Resolved path of the active theme file; empty if none.
    long long   themeStamp_ = 0;       ///< File timestamp at the last successful load.
    unsigned    themeRevision_ = 0;
    double      nextThemeCheck_ = 0.0;
    std::string pendingThemePath_;
    std::string pendingThemeText_;
    bool        hasPendingThemeText_ = false;
    float       dpiScale_ = 1.0f;
};

}  // namespace gui

/// Defines main() for a default-constructible App subclass.
#define GUI_MAIN(AppClass, Title, Width, Height) \
    int main() {                                 \
        gui::AppConfig cfg;                      \
        cfg.title  = Title;                      \
        cfg.width  = Width;                      \
        cfg.height = Height;                     \
        AppClass app;                            \
        return app.Run(cfg);                     \
    }
