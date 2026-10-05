<div align="center">
  <img alt="lw_gui" height="75" src="https://github.com/user-attachments/assets/8f5d9a6e-ac34-4ab5-8c72-59bc822b8643" />
</div>
</br>
<div align="center">

  ![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/lowersoft/gui/cmake-multi-platform.yml)
  ![GitHub License](https://img.shields.io/github/license/lowersoft/gui)
  ![GitHub Repo stars](https://img.shields.io/github/stars/lowersoft/gui)

</div>

LowerSoft GUI is a cross-platform user interface foundation for *Windows* and *Linux*, built on top of ![Dear ImGui](https://github.com/ocornut/imgui). Rather than maintaining a fork, it integrates with the upstream **ImGui API directly**, which keeps it easy to *update* and *free of divergence* from the original library.<br>
<sub>It serves as the common GUI layer for LowerSoft's commercial products and is under active development.</sub>

<hr>
<div align="center">
  <a href="#usage">Usage</a> · 
  <a href="#how-it-works-explained-for-just-project-infrastructure">How it Works</a> · 
  <a href="#contact">Contact</a> ·
  <a href="#credits">Credits</a> ·
  <a href="#contribute">Contribute</a> ·
  <a href="#license">License</a>
</div>
<hr>

### Usage
Everything below works the same on Windows (MSYS2 UCRT64) and Linux. No Visual Studio needed(btw).
#### 1.1 Get the tools
| Platform | One-time setup |
|-|---|
| Windows | Install [MSYS2](https://www.msys2.org), open the **MSYS2 UCRT64** terminal (not "MSYS"), then run `./scripts/setup-msys2.sh` |
| Linux (Arch Linux, Ubuntu, Debian and Fedora) | `./scripts/setup-linux.sh` |

The scripts install CMake, Ninja, a compiler and the OpenGL/X11 headers. Dear ImGui and GLFW are fetched automatically on the first configure, so there is nothing else to install.

#### 1.2 Build and run
```bash
git clone https://github.com/lowersoft/gui.git && cd gui
./build.sh release --run
```
`build.sh` takes `debug`, `release`, `clang-debug` or `clang-release`. The binaries land in `build/<preset>/bin/`. Prefer plain CMake? <3
```bash
cmake --preset release
cmake --build --preset release
```
> Debug builds read `conf/` straight from the source tree, so theme edits show up instantly. Release builds use the copy next to the executable.

#### 1.3 Write your first app
Derive from `gui::App`, override what you need, and write plain Dear ImGui inside `OnGui()`. There is no wrapper API to learn(we suggest ImGui Docs, these docs realy enough for this infrastructure; no needs to learn "ImGui Backend". It's realy will be easier than without infra).
```cpp
#include "gui/app.h"

class MyApp : public gui::App {
    void OnGui() override {
        ImGui::Begin("Hello");
        if (ImGui::Button("Quit")) RequestClose();
        ImGui::End();
    }
};

GUI_MAIN(MyApp, "My App", 1280, 720)
```
Register it in `examples/<name>/CMakeLists.txt` (one line) and add the folder to the root `CMakeLists.txt`:
```cmake
gui_add_app(myapp main.cpp)
```
Other hooks: `OnInit`, `OnUpdate`, `OnShutdown`. Window options such as docking, viewports, vsync and fonts live in `gui::AppConfig`. For a full tour, see the [framework reference](docs/framework.md); `examples/hello` and `examples/file_transfer` are good starting points.

#### 1.4 Theming
The whole look is driven by [`conf/theme.ini`](conf/theme.ini): a palette, every ImGui color and the style metrics. Save the file and the running app updates within half a second. Ready-made themes are in `conf/themes/`.
```cpp
SetTheme("conf/themes/light.ini");   // switch at runtime ("i'm not using light theme btw ;)")
```
Prefer a GUI? The `hello` example has a built-in Style Editor and a live text editor for the theme file.

#### 1.5 Handy options
| CMake option | Default | What it does |
|---|---|---|
| `GUI_BUILD_EXAMPLES` | `ON` | Build the example apps |
| `GUI_STATIC_RUNTIME` | `ON` | MinGW: no runtime DLLs to ship |
| `GUI_HIDE_CONSOLE` | `OFF` | Windows: no console window next to the app |

Pass them with `-D`, e.g. `cmake --preset release -DGUI_HIDE_CONSOLE=ON`.

Something broke? Check that you are in the **UCRT64** shell on Windows, delete `build/` and try again. Still stuck? [Open an issue](https://github.com/lowersoft/gui/issues).

### How it Works (explained for just project infrastructure)
### Contact
### Credits
### Contribute
### License
