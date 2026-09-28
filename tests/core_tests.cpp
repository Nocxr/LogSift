#include "config/config_store.h"
#include "core/process.h"
#include "ui/popup_timer.h"

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
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
    return ok ? 0 : 1;
}
