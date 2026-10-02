#include "gui/app.h"

#include <cstdio>
#include <filesystem>
#include <utility>

#include <GLFW/glfw3.h>

#include "paths.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace gui {
namespace {

void GlfwErrorCallback(int code, const char* description) {
    std::fprintf(stderr, "[glfw] error %d: %s\n", code, description);
}

std::string FindSystemFont() {
    static const char* const kCandidates[] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
    };
    for (const char* path : kCandidates) {
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) return path;
    }
    return {};
}

void LoadFont(const AppConfig& cfg) {
    ImGuiIO& io = ImGui::GetIO();
    const std::string path = cfg.fontPath.empty() ? FindSystemFont() : cfg.fontPath;

    if (!path.empty()) {
        if (io.Fonts->AddFontFromFileTTF(path.c_str(), cfg.fontSize)) return;
        std::fprintf(stderr, "[gui] failed to load font '%s', falling back to the built-in font\n", path.c_str());
    }

    ImFontConfig fontConfig;
    fontConfig.SizePixels = cfg.fontSize;
    io.Fonts->AddFontDefault(&fontConfig);
}

}  // namespace

void App::RequestClose() {
    if (window_) glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void App::SetTheme(const std::string& path) {
    pendingThemePath_ = path;
}

void App::ReloadTheme() {
    if (!themeFile_.empty()) pendingThemePath_ = themeFile_;
}

void App::PreviewThemeText(const std::string& text) {
    pendingThemeText_    = text;
    hasPendingThemeText_ = true;
}

void App::ApplyTheme(const Theme& theme) {
    theme.Apply(dpiScale_);
    if (config_.viewports) {
        // Detached windows are drawn by the OS, so square corners and an opaque background look consistent.
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

bool App::LoadTheme(const std::string& path) {
    const std::string resolved = detail::ResolveConfigPath(path);

    Theme candidate;
    if (!candidate.LoadFromFile(resolved)) {
        std::fprintf(stderr, "[theme] cannot read '%s'\n", resolved.c_str());
        return false;
    }
    for (const std::string& warning : candidate.Warnings()) std::fprintf(stderr, "[theme] %s\n", warning.c_str());

    theme_       = std::move(candidate);
    themeFile_   = resolved;
    themeStamp_  = detail::FileTimestamp(resolved);
    ++themeRevision_;
    ApplyTheme(theme_);
    return true;
}

// Runs between frames so style changes never land in the middle of a frame.
void App::UpdateTheme() {
    // A file request supersedes any pending preview.
    if (!pendingThemePath_.empty()) {
        const std::string path = std::move(pendingThemePath_);
        pendingThemePath_.clear();
        hasPendingThemeText_ = false;
        LoadTheme(path);
        return;
    }

    if (hasPendingThemeText_) {
        hasPendingThemeText_ = false;
        Theme preview;
        preview.LoadFromString(pendingThemeText_, "editor");
        theme_ = std::move(preview);
        ApplyTheme(theme_);
        return;
    }

    if (!config_.themeHotReload || themeFile_.empty()) return;

    const double now = glfwGetTime();
    if (now < nextThemeCheck_) return;
    nextThemeCheck_ = now + 0.5;

    const long long stamp = detail::FileTimestamp(themeFile_);
    if (stamp != 0 && stamp != themeStamp_) LoadTheme(themeFile_);
}

int App::Run(const AppConfig& config) {
    config_ = config;

    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit()) return 1;

    // Context versions and GLSL strings mirror the official ImGui GLFW+OpenGL3 example.
#if defined(__APPLE__)
    const char* glslVersion = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    float xScale = 1.0f, yScale = 1.0f;
    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) glfwGetMonitorContentScale(monitor, &xScale, &yScale);
    const float dpiScale = xScale > 0.0f ? xScale : 1.0f;

    // macOS window sizes are already expressed in points, so no extra scaling is applied there.
#if defined(__APPLE__)
    const float windowScale = 1.0f;
#else
    const float windowScale = dpiScale;
#endif

    window_ = glfwCreateWindow(static_cast<int>(static_cast<float>(config_.width) * windowScale),
                               static_cast<int>(static_cast<float>(config_.height) * windowScale),
                               config_.title.c_str(), nullptr, nullptr);
    if (!window_) {
        std::fprintf(stderr, "[gui] failed to create the window or OpenGL context; check the GPU driver\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(config_.vsync ? 1 : 0);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (config_.docking)   io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (config_.viewports) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    if (!config_.saveLayout) io.IniFilename = nullptr;

    dpiScale_ = dpiScale;
    if (config_.themePath.empty() || !LoadTheme(config_.themePath)) {
        Theme fallback;
        fallback.SetBase(config_.darkTheme ? ThemeBase::Dark : ThemeBase::Light);
        ApplyTheme(fallback);
    }

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glslVersion);
    LoadFont(config_);

    OnInit();

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        UpdateTheme();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (config_.docking && config_.dockspace)
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        const double now = glfwGetTime();
        OnUpdate(static_cast<float>(now - lastTime));
        lastTime = now;
        OnGui();

        ImGui::Render();
        int framebufferWidth = 0, framebufferHeight = 0;
        glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        const ImVec4& clear = config_.clearColor;
        glClearColor(clear.x * clear.w, clear.y * clear.w, clear.z * clear.w, clear.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            // Rendering platform windows switches the current context; restore it before swapping.
            GLFWwindow* mainContext = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(mainContext);
        }
        glfwSwapBuffers(window_);
    }

    OnShutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window_);
    window_ = nullptr;
    glfwTerminate();
    return 0;
}

}  // namespace gui
