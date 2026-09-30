add_library(gui_framework STATIC
    src/app.cpp
    src/theme.cpp
    src/ini.cpp
    src/paths.cpp)
add_library(gui::framework ALIAS gui_framework)

target_include_directories(gui_framework PUBLIC include)
target_link_libraries(gui_framework PUBLIC imgui)
target_compile_features(gui_framework PUBLIC cxx_std_17)
gui_set_warnings(gui_framework)

# Debug builds read conf/ straight from the source tree so theme edits hot-reload without copying.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(gui_framework PRIVATE GUI_DEV_ROOT="${PROJECT_SOURCE_DIR}")
endif()

# gui_add_app(<target> <sources...>)
# Creates an executable linked against the framework with the project's warning
# set and platform-specific link options applied.
function(gui_add_app target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} PRIVATE gui::framework)
    gui_set_warnings(${target})

    # Ship the configuration directory next to the executable.
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${PROJECT_SOURCE_DIR}/conf" "$<TARGET_FILE_DIR:${target}>/conf")

    if(WIN32)
        if(GUI_HIDE_CONSOLE)
            set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE TRUE)
        endif()
        # Keeps the executable independent of the MSYS2 runtime DLLs.
        if(MINGW AND GUI_STATIC_RUNTIME)
            target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
        endif()
    endif()
endfunction()
