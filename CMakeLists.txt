cmake_minimum_required(VERSION 3.20)
project(FastEnoughMac)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES arm64)

# Find GLFW (installed via brew)
find_package(glfw3 3.3 REQUIRED)

# OpenGL is available via Apple framework
find_package(OpenGL REQUIRED)

# ---- Dear ImGui ----
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/thirdparty/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# ---- MotionCam ----
set(MOTIONCAM_SOURCES
    thirdparty/motioncam/lib/Decoder.cpp
    thirdparty/motioncam/lib/RawData.cpp
    thirdparty/motioncam/lib/RawData_Legacy.cpp
)

# ---- App sources ----
# On macOS we use main_mac.cpp instead of main.cpp
# But we also patch device_client etc via #ifdef __APPLE__
set(APP_SOURCES
    src/app.cpp
    src/backup_archive.cpp
    src/device_client.cpp
    src/mcraw_local.cpp
)

# Dokan and ProjFS are Windows-only; provide stubs on macOS
if(APPLE)
    list(APPEND APP_SOURCES
        src/device_dokan_stub.cpp
        src/mcraw_projfs_stub.cpp
        src/main_mac.cpp
        src/trash_macos.mm
    )
else()
    list(APPEND APP_SOURCES
        src/device_dokan.cpp
        src/mcraw_projfs.cpp
        src/main.cpp
    )
endif()

add_executable(FastEnoughMac
    ${IMGUI_SOURCES}
    ${MOTIONCAM_SOURCES}
    ${APP_SOURCES}
)

target_include_directories(FastEnoughMac PRIVATE
    src
    thirdparty/imgui
    thirdparty/imgui/backends
    thirdparty/motioncam/include
    thirdparty
    thirdparty/nlohmann
    thirdparty/tinydng
    thirdparty/audiofile
)

target_link_libraries(FastEnoughMac PRIVATE
    glfw
    OpenGL::GL
)

# macOS frameworks
if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    target_link_libraries(FastEnoughMac PRIVATE
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
    )
endif()

target_compile_definitions(FastEnoughMac PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

# Warnings
if(APPLE)
    target_compile_options(FastEnoughMac PRIVATE -Wno-deprecated-declarations)
endif()

# Bundle properties for .app (optional)
if(APPLE)
    set_target_properties(FastEnoughMac PROPERTIES
        MACOSX_BUNDLE FALSE
        OUTPUT_NAME "FastEnough"
    )
endif()
