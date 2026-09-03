#include "app.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

// Globals mirroring Windows main.cpp API
bool g_reallyQuit = false;
GLFWwindow* g_mainWindow = nullptr;

void updateTaskbarProgress(float fraction, bool active) { (void)fraction; (void)active; }
void setTaskbarError() {}
void setTaskbarPaused() {}
void initTaskbarProgress(void* hwnd) { (void)hwnd; }
void* g_mainHwnd = nullptr;

// ---- Helpers ----
static std::string getSettingsPath() {
#ifdef __APPLE__
    const char* home = getenv("HOME");
    std::string dir = std::string(home ? home : "/tmp") + "/Library/Application Support/FastEnough";
    std::filesystem::create_directories(dir);
    return dir + "/window.cfg";
#else
    return "";
#endif
}

struct WindowPlacement {
    int x = 100, y = 100, w = 1400, h = 800;
    bool maximized = false;
};

static void saveWindowState(GLFWwindow* window) {
    if (!window) return;
    int x, y, w, h;
    glfwGetWindowPos(window, &x, &y);
    glfwGetWindowSize(window, &w, &h);
    if (w < 200 || h < 200) return;
    WindowPlacement wp{x, y, w, h, glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE};
    std::string path = getSettingsPath();
    if (path.empty()) return;
    std::ofstream f(path, std::ios::binary);
    if (f) f.write((const char*)&wp, sizeof(wp));
}

static bool loadWindowState(GLFWwindow* window) {
    std::string path = getSettingsPath();
    if (path.empty()) return false;
    std::ifstream f(path, std::ios::binary);
    WindowPlacement wp{};
    if (f && f.read((char*)&wp, sizeof(wp))) {
        if (wp.w < 200 || wp.h < 200) return false;
        glfwSetWindowPos(window, wp.x, wp.y);
        glfwSetWindowSize(window, wp.w, wp.h);
        if (wp.maximized) glfwMaximizeWindow(window);
        return true;
    }
    return false;
}

static std::string getExeSidePath(const char* fileName) {
#ifdef __APPLE__
    char path[4096] = {};
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::string p(path);
        auto slash = p.find_last_of('/');
        if (slash != std::string::npos) return p.substr(0, slash+1) + fileName;
    }
    return std::string(fileName);
#else
    return fileName;
#endif
}

static std::string WideToUtf8Dummy(const std::wstring& v) { (void)v; return ""; }

static void glfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Drop callback - forward to app
static App* g_app = nullptr;
static void dropCallback(GLFWwindow* window, int count, const char** paths) {
    if (!g_app || count <=0) return;
    std::vector<std::string> vec;
    for (int i=0;i<count;i++) vec.emplace_back(paths[i] ? paths[i] : "");
    // get mouse pos for targeting which panel
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);
    g_app->handleExternalFileDrop(vec, (int)mx, (int)my);
}

static std::string g_crashLogPath;

int main(int argc, char** argv) {
    g_crashLogPath = getExeSidePath("Fast Enough - Android File Explorer.log");
    DebugLog::instance().setFilePath(g_crashLogPath);
    LOG_INFO("Startup", "FastEnough macOS starting. Log: " + g_crashLogPath);

    std::string launchApkPath;
    for (int i=1;i<argc;i++) {
        std::string a(argv[i]);
        if (a.size()>=4 && (a.substr(a.size()-4)==".apk" || a.substr(a.size()-4)==".APK")) {
            launchApkPath = a;
            break;
        }
    }

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1400, 800, "Fast Enough - Android File Explorer", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    g_mainWindow = window;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetDropCallback(window, dropCallback);

    // Restore window placement
    loadWindowState(window);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Ini file path -> ~/Library/Application Support/FastEnough/imgui.ini
    static std::string imguiIniPath;
    {
        std::string settingsPath = getSettingsPath(); // .../window.cfg
        auto slash = settingsPath.rfind('/');
        if (slash != std::string::npos) imguiIniPath = settingsPath.substr(0, slash) + "/imgui.ini";
        else imguiIniPath = "imgui.ini";
    }
    io.IniFilename = imguiIniPath.c_str();

    // DPI scale - on macOS, GLFW/ImGui handle Retina via DisplayFramebufferScale,
    // so we keep dpiScale=1.0 to avoid double-scaling (menus overlapping).
    float dpiScale = 1.0f;
    float fontSize = 16.0f;
    ImFontConfig fontCfg;
    fontCfg.OversampleH = 3;
    fontCfg.OversampleV = 3;
    fontCfg.PixelSnapH = true;
    fontCfg.RasterizerDensity = 1.0f;

    // Try to load system font - on mac, use Helvetica or SF
    ImFont* mainFont = nullptr;
    // On mac, ImGui default font is fine; optionally load from /System/Library/Fonts
    const char* macFonts[] = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/System/Library/Fonts/SFNSDisplay.ttf",
        nullptr
    };
    for (int i=0; macFonts[i]; ++i) {
        if (std::filesystem::exists(macFonts[i])) {
            mainFont = io.Fonts->AddFontFromFileTTF(macFonts[i], fontSize, &fontCfg);
            if (mainFont) break;
        }
    }
    if (!mainFont) {
        fontCfg.SizePixels = fontSize;
        mainFont = io.Fonts->AddFontDefault(&fontCfg);
    }
    io.Fonts->Build();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    App app;
    app.m_systemDpiScale = dpiScale;
    app.applyScale(app.m_theme.userScale);
    if (!launchApkPath.empty()) app.requestApkInstall(launchApkPath);
    g_app = &app;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Handle pending scale change
        if (g_app && g_app->m_pendingScale > 0) {
            g_app->applyScale(g_app->m_pendingScale);
            g_app->m_scalePreview = g_app->m_theme.userScale;
            g_app->m_theme.save();
            g_app->m_pendingScale = -1.0f;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        g_app->render();
        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        // Sleep if no high FPS needed
        if (!app.wantsHighFps()) {
            // glfwPollEvents already handles waiting; sleep a bit to reduce CPU
            // Use glfwWaitEventsTimeout for idle
            glfwWaitEventsTimeout(0.033);
        }
    }

    saveWindowState(window);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    g_app = nullptr;
    return 0;
}
