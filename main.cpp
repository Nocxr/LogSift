#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nlohmann/json.hpp>
#include "src/config/config_types.h"
#include "src/core/process.h"

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
#include "src/ui/style.inl"
#include "src/core/activity.inl"
#include "src/config/config_store.inl"
#include "src/core/output_prefs.inl"
#include "src/audio/notifications.inl"
#include "src/core/stats_history.inl"
#include "src/core/sift_types.inl"
#include "src/profiles/profiles.inl"
#include "src/model/client.inl"
#include "src/model/fast_path.inl"
#include "src/model/vision.inl"
#include "src/io/input.inl"
#include "src/ui/widgets.inl"

}

#include "src/cli/help.inl"

int main(int argc, char** argv) {
#include "src/cli/run.inl"
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

