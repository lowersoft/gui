cmake_minimum_required(VERSION 3.20)

project(ImGuiFramework VERSION 1.0.0 LANGUAGES C CXX)

# The MSYS2 "MSYS" environment targets the Cygwin runtime, which is not supported.
if(CYGWIN OR MSYS)
    message(FATAL_ERROR
        "Unsupported MSYS2 environment: 'MSYS' uses the Cygwin runtime.\n"
        "Open the 'MSYS2 UCRT64' terminal and configure again.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(GUI_BUILD_EXAMPLES "Build the example applications"                    ON)
option(GUI_STATIC_RUNTIME "MinGW: link libstdc++, libgcc and winpthread statically" ON)
option(GUI_HIDE_CONSOLE   "Windows: do not open a console window alongside the app" OFF)

include(cmake/CompilerWarnings.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(framework)

if(GUI_BUILD_EXAMPLES)
    add_subdirectory(examples/hello)
endif()
