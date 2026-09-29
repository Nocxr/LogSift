#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nlohmann/json.hpp>
#include "src/model/http.h"
#include "src/model/client.h"
#include "src/profiles/profiles.h"
#include "src/ui/popup_timer.h"
#include "src/ui/style.h"
#include "src/config/config_types.h"
#include "src/config/config_store.h"
#include "src/core/stats_history.h"
#include "src/core/process.h"
#include "src/core/task.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <atomic>
#include <memory>
#include <functional>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <thread>

#ifndef LOGSIFT_VERSION
#define LOGSIFT_VERSION "dev"
#endif
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <commdlg.h>

std::vector<std::string> WindowsCommandLineArgs() {
    int wideArgc = 0;
    LPWSTR* wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);
    if (!wideArgv || wideArgc <= 0) {
        if (wideArgv) LocalFree(wideArgv);
        return {"logsift"};
    }

    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(wideArgc));
    for (int i = 0; i < wideArgc; ++i) {
        const int bytes = WideCharToMultiByte(
            CP_UTF8, 0, wideArgv[i], -1, nullptr, 0, nullptr, nullptr);
        if (bytes <= 0) {
            args.emplace_back();
            continue;
        }

        std::string arg(static_cast<size_t>(bytes), '\0');
        WideCharToMultiByte(
            CP_UTF8, 0, wideArgv[i], -1, arg.data(), bytes, nullptr, nullptr);
        if (!arg.empty() && arg.back() == '\0') arg.pop_back();
        args.push_back(std::move(arg));
    }

    LocalFree(wideArgv);
    return args;
}

bool WindowsCliRequested(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (!argv[i]) continue;
        if (std::string(argv[i]) != "--background") return true;
    }
    return false;
}

void WindowsRebindConsoleStream(
    FILE* stream, DWORD standardHandle, const char* device, const char* mode) {
    const HANDLE handle = GetStdHandle(standardHandle);
    if (!handle || handle == INVALID_HANDLE_VALUE) return;

    DWORD consoleMode = 0;
    if (!GetConsoleMode(handle, &consoleMode)) return;

#ifdef _MSC_VER
    FILE* reopened = nullptr;
    (void)freopen_s(&reopened, device, mode, stream);
#else
    (void)freopen(device, mode, stream);
#endif
}

void WindowsAttachParentConsoleForCli(int argc, char** argv) {
    if (!WindowsCliRequested(argc, argv)) return;

    if (!AttachConsole(ATTACH_PARENT_PROCESS) &&
        GetLastError() != ERROR_ACCESS_DENIED) {
        return;
    }

    // Preserve redirected file/pipe handles. Only reconnect CRT streams that
    // actually point at the attached console.
    WindowsRebindConsoleStream(stdin, STD_INPUT_HANDLE, "CONIN$", "r");
    WindowsRebindConsoleStream(stdout, STD_OUTPUT_HANDLE, "CONOUT$", "w");
    WindowsRebindConsoleStream(stderr, STD_ERROR_HANDLE, "CONOUT$", "w");
    std::ios::sync_with_stdio(true);
}
#endif

#ifdef __APPLE__
extern "C" void LogSiftMacTrayInit(void);
extern "C" bool LogSiftMacTrayTakeOpen(void);
extern "C" bool LogSiftMacTrayTakeToggleWatch(void);
extern "C" bool LogSiftMacTrayTakeToggleOcr(void);
extern "C" bool LogSiftMacTrayTakeToggleAutoCopy(void);
extern "C" bool LogSiftMacTrayTakeToggleSound(void);
extern "C" bool LogSiftMacTrayTakeOpenLog(void);
extern "C" bool LogSiftMacTrayTakeCopy(void);
extern "C" bool LogSiftMacTrayTakeQuit(void);
extern "C" void LogSiftMacTraySetWatch(bool enabled);
extern "C" void LogSiftMacTraySetOcr(bool enabled);
extern "C" void LogSiftMacTraySetAutoCopy(bool enabled);
extern "C" void LogSiftMacTraySetSound(bool enabled);
extern "C" long long LogSiftMacClipboardChangeCount(void);
extern "C" bool LogSiftMacGetStartAtLogin(void);
extern "C" bool LogSiftMacSetStartAtLogin(bool enabled);
extern "C" bool LogSiftMacActivateExistingInstance(void);
#endif

namespace {
using json = nlohmann::json;


#include "src/platform/windows.inl"
#include "src/platform/clipboard.inl"
#include "src/core/activity.inl"
#include "src/core/output_prefs.inl"
#include "src/audio/notifications.inl"
#include "src/core/sift_types.inl"
#include "src/profiles/profiles.inl"
#include "src/model/fast_path.inl"
#include "src/model/vision.inl"
#include "src/io/input.inl"
#include "src/ui/widgets.inl"
#include "src/app/state.inl"

}

#include "src/cli/help.inl"
#include "src/cli/run.inl"

int RunLogSift(int argc, char** argv) {
#ifdef _WIN32
    WindowsAttachParentConsoleForCli(argc, argv);
#endif
    const CliDispatch cli = RunCli(argc, argv);
    if (cli.handled) return cli.exitCode;
    const bool backgroundMode = cli.backgroundMode;
#include "src/app/setup.inl"
#include "src/app/events_tray_watch.inl"
#include "src/app/events_sdl_async.inl"
#include "src/ui/main_header.inl"
#include "src/ui/sift_tab.inl"
#include "src/ui/recents_tab.inl"
#include "src/ui/settings_tab.inl"
#include "src/ui/main_render.inl"
#include "src/ui/toast_lifecycle.inl"
#include "src/ui/toast_layout.inl"
#include "src/ui/toast_content.inl"
#include "src/ui/toast_footer.inl"
#include "src/app/frame_wait.inl"
#include "src/app/shutdown.inl"

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    std::vector<std::string> args = WindowsCommandLineArgs();
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (std::string& arg : args) argv.push_back(arg.data());
    return RunLogSift(static_cast<int>(argv.size()), argv.data());
}
#else
int main(int argc, char** argv) {
    return RunLogSift(argc, argv);
}
#endif

