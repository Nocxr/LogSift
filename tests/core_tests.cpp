#include "config/config_store.h"
#include "core/process.h"
#include "core/stats_history.h"
#include "core/task.h"
#include "ui/popup_timer.h"
#include "profiles/profiles.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    auto check = [](bool ok, const char* message) {
        if (!ok) std::cerr << "FAILED: " << message << '\n';
        return ok;
    };
    bool ok = true;
    ok &= check(Base64Encode({'f', 'o', 'o'}) == "Zm9v", "base64");
    ok &= check(DedupeLines("a\na\nb\n") == "a\nb\n", "deduplicate");
    Config config;
    config.showTimestamps = false;
    ok &= check(FormatDiagnosticText(
        "[2026.09.26-03.21.13:141][585]LogTest: error\n", config) ==
        "LogTest: error\n", "timestamp formatting");
    ok &= check(IsEndpointUnavailableError("curl failed (exit 7)"), "offline detection");
    auto task = LaunchBackgroundTask([] { return 42; });
    ok &= check(task.get() == 42, "background task result");

    LogProfile generic;
    generic.id = "generic";
    generic.name = "Generic";
    LogProfile compiler;
    compiler.id = "msvc";
    compiler.name = "MSVC";
    compiler.detect = {"error C"};
    LogProfile cmake;
    cmake.id = "cmake-ninja";
    cmake.name = "CMake / Ninja";
    cmake.detect = {"FAILED:", "ninja: build stopped"};
    gProfiles = {generic, compiler, cmake};
    ok &= check(DetectProfile("C:\\\\src\\\\a.cpp(2): error C2143", config)->id == "msvc",
                "profile detection");
    const auto mixedProfiles = DetectProfiles(
        "FAILED: target.obj\n"
        "cl.exe /c a.cpp\n"
        "C:\\\\src\\\\a.cpp(2): error C2143\n"
        "ninja: build stopped: subcommand failed.\n",
        config);
    bool foundMsvc = false;
    bool foundCmake = false;
    for (const LogProfile* profile : mixedProfiles) {
        if (!profile) continue;
        foundMsvc = foundMsvc || profile->id == "msvc";
        foundCmake = foundCmake || profile->id == "cmake-ninja";
    }
    ok &= check(foundMsvc && foundCmake && mixedProfiles.size() == 2,
                "additive mixed build profile detection");
    gProfiles.clear();

    using Clock = std::chrono::steady_clock;
    const auto start = Clock::time_point(std::chrono::seconds(10));
    auto until = start + std::chrono::seconds(5);
    PopupTimerState timer;
    ok &= check(AdvancePopupTimer(timer, start, start, until, false, true, true, false),
                "popup starts active");
    ok &= check(AdvancePopupTimer(timer, start + std::chrono::seconds(1),
                                start, until, false, true, true, true) && timer.paused,
                "popup pauses on hover");
    ok &= check(AdvancePopupTimer(timer, start + std::chrono::seconds(6),
                                start, until, false, true, true, false) && timer.paused,
                "popup keeps hover grace");
    ok &= check(AdvancePopupTimer(timer, start + std::chrono::seconds(8),
                                start, until, false, true, true, false) && !timer.paused,
                "popup resumes after grace");
    ok &= check(!AdvancePopupTimer(timer, until, start, until, false, true, true, false),
                "popup expires");
    const auto nextShown = until + std::chrono::seconds(1);
    until = nextShown + std::chrono::seconds(4);
    ok &= check(AdvancePopupTimer(timer, nextShown, nextShown, until,
                                false, true, true, false) && !timer.paused,
                "new popup resets hold");
    ok &= check(AdvancePopupTimer(timer, nextShown + std::chrono::seconds(1),
                                nextShown, until, false, true, true, true) && timer.paused,
                "new popup can pause on hover");
    ok &= check(!AdvancePopupTimer(timer, nextShown + std::chrono::seconds(2),
                                 nextShown, until, false, false, false, false) &&
                    !timer.paused && !timer.resumeRequested,
                "dismissing hovered popup clears paused state");

    const auto directory = std::filesystem::temp_directory_path() /
        ("logsift-tests-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto path = directory / "settings.json";
    config.model = "test/model";
    SaveConfigFile(path, config);
    Config restored;
    LoadConfigFile(path, restored);
    ok &= check(restored.model == "test/model", "settings round trip");
    config.model = "second/model";
    SaveConfigFile(path, config);
    LoadConfigFile(path, restored);
    ok &= check(restored.model == "second/model", "settings replacement");
    for (const auto& file : std::filesystem::directory_iterator(directory))
        ok &= check(file.path() == path, "no abandoned temporary settings");
    {
        std::ofstream invalid(path, std::ios::trunc);
        invalid << "{malformed";
    }
    Config fallback;
    LoadConfigFile(path, fallback);
    ok &= check(fallback.model == Config{}.model, "malformed settings fallback");
    {
        std::ofstream invalid(path, std::ios::trunc);
        invalid << R"({"model":"partial","recent_limit":"invalid"})";
    }
    LoadConfigFile(path, fallback);
    ok &= check(fallback.model == Config{}.model, "invalid field cannot partially apply");
    RecentRun run;
    run.input = "build output";
    run.output = "error C2143";
    run.ocr.present = true;
    run.ocr.imageBytes = 128;
    const auto recents = directory / "recents.json";
    SaveRecentRunsFile(recents, {run});
    const auto loadedRuns = LoadRecentRunsFile(recents, 5);
    ok &= check(loadedRuns.size() == 1 && loadedRuns[0].output == run.output &&
                loadedRuns[0].ocr.imageBytes == 128, "recents round trip");
    SaveRecentRunsFile(recents, {run, run});
    ok &= check(LoadRecentRunsFile(recents, 1).size() == 1,
                "recents limit and replacement");
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
    return ok ? 0 : 1;
}
