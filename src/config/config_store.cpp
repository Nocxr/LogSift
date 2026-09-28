#include "config_store.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

using json = nlohmann::json;

// Settings paths, JSON serialization, loading, saving, profile seeding.
// Compiled separately from the application entry point.

std::filesystem::path UserDataDir() {
#ifdef _WIN32
    char* homeBuffer = nullptr;
    size_t homeLength = 0;
    const errno_t homeResult = _dupenv_s(&homeBuffer, &homeLength, "USERPROFILE");
    std::filesystem::path base =
        (homeResult == 0 && homeBuffer && *homeBuffer)
            ? std::filesystem::path(homeBuffer)
            : std::filesystem::current_path();
    std::free(homeBuffer);
#else
    const char* home = std::getenv("HOME");
    std::filesystem::path base =
        (home && *home) ? std::filesystem::path(home) : std::filesystem::current_path();
#endif
    return base / ".logsift";
}

std::filesystem::path SettingsPath() {
    return UserDataDir() / "settings.json";
}

json ConfigToJson(const Config& cfg) {
    return json{
        {"endpoint", cfg.endpoint},
        {"model", cfg.model},
        {"api_key", cfg.apiKey},
        {"profile_id", cfg.profileId},
        {"compute_mode", cfg.computeMode},
        {"show_errors", cfg.showErrors},
        {"show_warnings", cfg.showWarnings},
        {"show_context", cfg.showContext},
        {"show_known_noise", cfg.showKnownNoise},
        {"show_timestamps", cfg.showTimestamps},
        {"group_diagnostics", cfg.groupDiagnostics},
        {"toast_bg", {cfg.toastBg[0], cfg.toastBg[1], cfg.toastBg[2]}},
        {"toast_accent", {cfg.toastAccent[0], cfg.toastAccent[1], cfg.toastAccent[2]}},
        {"toast_seconds", cfg.toastSeconds},
        {"toast_fps", cfg.toastFps},
        {"toast_sound", cfg.toastSound},
        {"start_sound_preset", cfg.startSoundPreset},
        {"end_sound_preset", cfg.endSoundPreset},
        {"offline_sound_preset", cfg.offlineSoundPreset},
        {"failure_sound_preset", cfg.failureSoundPreset},
        {"toast_sound_file", cfg.toastSoundFile},
        {"toast_show_type", cfg.toastShowType},
        {"toast_show_bytes", cfg.toastShowBytes},
        {"toast_show_time", cfg.toastShowTime},
        {"toast_show_counts", cfg.toastShowCounts},
        {"toast_show_preview", cfg.toastShowPreview},

        {"popup_scan_show_source", cfg.scanShowSource},
        {"popup_scan_show_model", cfg.scanShowModel},
        {"popup_scan_show_route", cfg.scanShowRoute},
        {"popup_scan_show_progress", cfg.scanShowProgress},
        {"popup_scan_show_prefilter_counts", cfg.scanShowPrefilterCounts},
        {"popup_scan_show_estimated_tokens", cfg.scanShowEstimatedTokens},
        {"popup_scan_show_bytes_reduction", cfg.scanShowBytesReduction},
        {"popup_scan_show_elapsed_time", cfg.scanShowElapsedTime},

        {"popup_result_show_diagnostic_total", cfg.resultShowDiagnosticTotal},
        {"popup_result_show_source", cfg.resultShowSource},
        {"popup_result_show_model", cfg.resultShowModel},
        {"popup_result_show_route", cfg.resultShowRoute},
        {"popup_result_show_fallback_notice", cfg.resultShowFallbackNotice},
        {"popup_result_show_prefilter_counts", cfg.resultShowPrefilterCounts},
        {"popup_result_show_estimated_tokens", cfg.resultShowEstimatedTokens},
        {"popup_result_show_real_tokens", cfg.resultShowRealTokens},
        {"popup_result_show_token_speed", cfg.resultShowTokenSpeed},
        {"popup_result_show_bytes_reduction", cfg.resultShowBytesReduction},
        {"popup_result_show_time", cfg.resultShowTime},
        {"popup_result_show_auto_copy", cfg.resultShowAutoCopy},
        {"popup_result_show_counts", cfg.resultShowCounts},
        {"popup_result_show_preview", cfg.resultShowPreview},
        {"popup_result_show_lifetime_bar", cfg.resultShowLifetimeBar},

        {"toast_preview_lines", cfg.toastPreviewLines},
        {"toast_acknowledge_clipboard", cfg.toastAcknowledgeClipboard},
        {"auto_copy_results", cfg.autoCopyResults},
        {"watch_clipboard", cfg.watchClipboard},
        {"ocr_enabled", cfg.ocrEnabled},
        {"auto_scan_images", cfg.autoScanImages},
        {"ocr_prompt_seconds", cfg.ocrPromptSeconds},
        {"ocr_prompt_show_image_details", cfg.ocrPromptShowImageDetails},
        {"ocr_prompt_show_timeout_bar", cfg.ocrPromptShowTimeoutBar},
        {"recent_limit", cfg.recentLimit},
        {"prefer_fast_path", cfg.preferFastPath}
    };
}

void LoadConfigFile(const std::filesystem::path& path, Config& cfg) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return;
    Config loaded = cfg;
    try {
        json j; in >> j;
        loaded.endpoint = j.value("endpoint", loaded.endpoint);
        loaded.model = j.value("model", loaded.model);
        loaded.apiKey = j.value("api_key", loaded.apiKey);
        loaded.profileId = j.value("profile_id", loaded.profileId);
        loaded.computeMode = j.value("compute_mode", loaded.computeMode);
        loaded.showErrors = j.value("show_errors", loaded.showErrors);
        loaded.showWarnings = j.value("show_warnings", loaded.showWarnings);
        loaded.showContext = j.value("show_context", loaded.showContext);
        loaded.showKnownNoise = j.value("show_known_noise", loaded.showKnownNoise);
        loaded.showTimestamps = j.value("show_timestamps", loaded.showTimestamps);
        loaded.groupDiagnostics = j.value("group_diagnostics", loaded.groupDiagnostics);
        loaded.toastSeconds = j.value("toast_seconds", loaded.toastSeconds);
        loaded.toastFps = j.value("toast_fps", loaded.toastFps);
        loaded.toastSound = j.value("toast_sound", loaded.toastSound);
        loaded.startSoundPreset = j.value("start_sound_preset", loaded.startSoundPreset);
        loaded.endSoundPreset = j.value("end_sound_preset", loaded.endSoundPreset);
        loaded.offlineSoundPreset = j.value("offline_sound_preset", loaded.offlineSoundPreset);
        loaded.failureSoundPreset = j.value("failure_sound_preset", loaded.failureSoundPreset);
        loaded.toastSoundFile = j.value("toast_sound_file", loaded.toastSoundFile);
        loaded.toastShowType = j.value("toast_show_type", loaded.toastShowType);
        loaded.toastShowBytes = j.value("toast_show_bytes", loaded.toastShowBytes);
        loaded.toastShowTime = j.value("toast_show_time", loaded.toastShowTime);
        loaded.toastShowCounts = j.value("toast_show_counts", loaded.toastShowCounts);
        loaded.toastShowPreview = j.value("toast_show_preview", loaded.toastShowPreview);

        loaded.scanShowSource = j.value("popup_scan_show_source", loaded.toastShowType);
        loaded.scanShowModel = j.value("popup_scan_show_model", true);
        loaded.scanShowRoute = j.value("popup_scan_show_route", true);
        loaded.scanShowProgress = j.value("popup_scan_show_progress", true);
        loaded.scanShowPrefilterCounts = j.value("popup_scan_show_prefilter_counts", loaded.toastShowBytes);
        loaded.scanShowEstimatedTokens = j.value("popup_scan_show_estimated_tokens", loaded.toastShowBytes);
        loaded.scanShowBytesReduction = j.value("popup_scan_show_bytes_reduction", loaded.toastShowBytes);
        loaded.scanShowElapsedTime = j.value("popup_scan_show_elapsed_time", loaded.toastShowTime);

        loaded.resultShowDiagnosticTotal = j.value("popup_result_show_diagnostic_total", true);
        loaded.resultShowSource = j.value("popup_result_show_source", loaded.toastShowType);
        loaded.resultShowModel = j.value("popup_result_show_model", true);
        loaded.resultShowRoute = j.value("popup_result_show_route", true);
        loaded.resultShowFallbackNotice = j.value("popup_result_show_fallback_notice", true);
        loaded.resultShowPrefilterCounts = j.value("popup_result_show_prefilter_counts", loaded.toastShowBytes);
        loaded.resultShowEstimatedTokens = j.value("popup_result_show_estimated_tokens", loaded.toastShowBytes);
        loaded.resultShowRealTokens = j.value("popup_result_show_real_tokens", loaded.toastShowBytes);
        loaded.resultShowTokenSpeed = j.value("popup_result_show_token_speed", true);
        loaded.resultShowBytesReduction = j.value("popup_result_show_bytes_reduction", loaded.toastShowBytes);
        loaded.resultShowTime = j.value("popup_result_show_time", loaded.toastShowTime);
        loaded.resultShowAutoCopy = j.value("popup_result_show_auto_copy", true);
        loaded.resultShowCounts = j.value("popup_result_show_counts", loaded.toastShowCounts);
        loaded.resultShowPreview = j.value("popup_result_show_preview", loaded.toastShowPreview);
        loaded.resultShowLifetimeBar = j.value("popup_result_show_lifetime_bar", true);

        loaded.toastPreviewLines = std::clamp(j.value("toast_preview_lines", loaded.toastPreviewLines), 1, 10);
        loaded.toastAcknowledgeClipboard = j.value("toast_acknowledge_clipboard", loaded.toastAcknowledgeClipboard);
        loaded.autoCopyResults = j.value("auto_copy_results", loaded.autoCopyResults);
        loaded.watchClipboard = j.value("watch_clipboard", loaded.watchClipboard);
        loaded.ocrEnabled = j.value("ocr_enabled", loaded.ocrEnabled);
        loaded.autoScanImages = j.value("auto_scan_images", loaded.autoScanImages);
        loaded.ocrPromptSeconds = std::clamp(
            j.value("ocr_prompt_seconds", loaded.ocrPromptSeconds), 2.0f, 60.0f);
        loaded.ocrPromptShowImageDetails =
            j.value("ocr_prompt_show_image_details", loaded.ocrPromptShowImageDetails);
        loaded.ocrPromptShowTimeoutBar =
            j.value("ocr_prompt_show_timeout_bar", loaded.ocrPromptShowTimeoutBar);
        loaded.recentLimit = std::clamp(
            j.value("recent_limit", loaded.recentLimit), 1, 50);
        loaded.preferFastPath = j.value("prefer_fast_path", loaded.preferFastPath);

        auto loadColor = [&](const char* key, float (&dst)[3]) {
            if (!j.contains(key) || !j[key].is_array() || j[key].size() < 3) return;
            for (int i = 0; i < 3; ++i) dst[i] = j[key][i].get<float>();
        };
        loadColor("toast_bg", loaded.toastBg);
        loadColor("toast_accent", loaded.toastAccent);
        cfg = std::move(loaded);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "logsift: could not read %s: %s\n",
                     path.string().c_str(), e.what());
    }
}

void LoadConfig(Config& cfg) { LoadConfigFile(SettingsPath(), cfg); }

void SaveConfigFile(const std::filesystem::path& target, const Config& cfg) {
    std::error_code ec;
    std::filesystem::create_directories(target.parent_path(), ec);
    if (ec) {
        std::fprintf(stderr, "logsift: could not create settings directory: %s\n",
                     ec.message().c_str());
        return;
    }

    // Write in the destination directory, then replace the old file. A failed
    // write leaves the previous settings intact.
    static std::atomic<unsigned long long> sequence{0};
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path temporary = target;
    temporary += ".tmp-" + std::to_string(nonce) +
        "-" + std::to_string(sequence.fetch_add(1));
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out || !(out << ConfigToJson(cfg).dump(2) << '\n') || !(out.flush())) {
            std::fprintf(stderr, "logsift: could not write settings file\n");
            std::filesystem::remove(temporary, ec);
            return;
        }
        out.close();
        if (!out) {
            std::fprintf(stderr, "logsift: could not close settings file\n");
            std::filesystem::remove(temporary, ec);
            return;
        }
    }
#ifndef _WIN32
    std::filesystem::permissions(temporary,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace, ec);
    if (ec) {
        std::fprintf(stderr, "logsift: could not set settings permissions: %s\n",
                     ec.message().c_str());
        std::filesystem::remove(temporary, ec);
        return;
    }
    std::filesystem::rename(temporary, target, ec);
#else
    if (!MoveFileExW(temporary.c_str(), target.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        ec = std::error_code(static_cast<int>(GetLastError()), std::system_category());
#endif
    if (ec) {
        std::fprintf(stderr, "logsift: could not replace settings file: %s\n",
                     ec.message().c_str());
        std::filesystem::remove(temporary, ec);
    }
}

void SaveConfig(const Config& cfg) { SaveConfigFile(SettingsPath(), cfg); }


void SeedUserProfiles(const char* argv0) {
    const std::filesystem::path dest = UserDataDir() / "profiles";
    std::error_code ec;
    std::filesystem::create_directories(dest, ec);

    std::vector<std::filesystem::path> sources;
    if (argv0 && *argv0)
        sources.push_back(std::filesystem::absolute(argv0).parent_path() / "log-sift-profiles");
    sources.push_back(std::filesystem::current_path() / "profiles");

    for (const auto& source : sources) {
        if (!std::filesystem::is_directory(source, ec)) continue;
        for (const auto& entry : std::filesystem::directory_iterator(source, ec)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
            const auto target = dest / entry.path().filename();
            if (!std::filesystem::exists(target, ec))
                std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::none, ec);
            ec.clear();
        }
    }
}

