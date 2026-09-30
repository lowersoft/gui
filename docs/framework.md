# ImGui Cross-Platform Framework

Dear ImGui (docking branch) with GLFW and OpenGL 3, built with CMake and Ninja. No Visual Studio required.
ImGui and GLFW are fetched automatically from GitHub during the first configure.

## Windows (MSYS2 UCRT64)

Use the **MSYS2 UCRT64** terminal. The MSYS, MINGW64 and CLANG64 environments are not supported.

```bash
cd /c/path/to/this/project
./scripts/setup-msys2.sh        # one-time toolchain install
./build.sh release --run
```

The executable is written to `build/release/bin/hello.exe`. libstdc++ and libgcc are linked statically,
so it does not depend on MSYS2 DLLs.

## Linux

```bash
./scripts/setup-linux.sh        # Debian/Ubuntu dependencies
./build.sh release --run
```

## Manual build

```bash
cmake --preset release
cmake --build --preset release --parallel
./build/release/bin/hello
```

Available presets: `debug`, `release`, `clang-debug`, `clang-release`.

## Writing an application

```cpp
#include "gui/app.h"

class MyApp : public gui::App {
    void OnGui() override {
        ImGui::Begin("Window");
        ImGui::Text("Hello");
        ImGui::End();
    }
};
GUI_MAIN(MyApp, "Title", 1280, 720)
```

Hooks: `OnInit()`, `OnUpdate(float dt)`, `OnGui()`, `OnShutdown()`.
Runtime options live in `gui::AppConfig` (vsync, docking, viewports, font, theme). To customise them,
write your own `main()` and call `app.Run(config)` instead of using `GUI_MAIN`.

To add an application, create `examples/<name>/CMakeLists.txt` containing `gui_add_app(<name> main.cpp)`
and register it with `add_subdirectory(examples/<name>)` in the root `CMakeLists.txt`.

## Theming

Colors and style metrics come from `conf/theme.ini` (hot-reloaded while the app runs).

```ini
[theme]
name = Midnight
base = dark                  ; dark | light | classic

[palette]                    ; named colors, referenced as $name
bg     = #12141A
accent = #5B8CFF

[colors]                     ; keys are ImGuiCol_ names (Text, WindowBg, Button, ...)
WindowBg      = $bg
Button        = $accent @0.6 ; "@alpha" multiplies the color's alpha
ButtonHovered = $accent
ChildBg       = #00000000    ; #RRGGBB or #RRGGBBAA

[style]                      ; ImGuiStyle metrics in logical pixels (scaled by DPI)
WindowRounding = 6
FramePadding   = 8, 5
```

- Keys are case-insensitive. Anything omitted keeps the value of the `base` theme, so partial files work
  (see `conf/themes/light.ini`).
- Invalid lines are skipped and reported with their line number on stderr and through
  `App::CurrentTheme().Warnings()`; the application never fails because of a theme file.
- `conf/` is copied next to the executable on every build. Debug builds read it directly from the source tree,
  so edits take effect immediately. For Release builds edit `build/<preset>/bin/conf/theme.ini`.
- Switch at runtime with `SetTheme("conf/themes/light.ini")` (applied at the start of the next frame) or
  `ReloadTheme()`. Configure the startup file with `AppConfig::themePath` (empty disables file theming)
  and `AppConfig::themeHotReload`.
- `Theme::Serialize(ImGui::GetStyle())` writes the complete current style as theme INI text, which is a
  convenient starting point for a new theme.

## CMake options

| Option | Default | Description |
|---|---|---|
| `GUI_BUILD_EXAMPLES` | ON | Build the applications under `examples/`. |
| `GUI_STATIC_RUNTIME` | ON | MinGW: link the C++ runtime statically. |
| `GUI_HIDE_CONSOLE` | OFF | Windows: run without a console window. |

## Troubleshooting

- **"Unsupported MSYS2 environment"**: open the UCRT64 terminal.
- **CMake cache path mismatch**: a build directory created from a different terminal is stale; delete `build/`.
- **GLFW reports no window system (Linux)**: install the X11/Wayland development packages via `setup-linux.sh`, then delete `build/`.
- **Fonts**: when `AppConfig::fontPath` is empty, a system font (Segoe UI, DejaVu, Noto, ...) is used. Place custom
  fonts under `assets/fonts/` and pass their path.
