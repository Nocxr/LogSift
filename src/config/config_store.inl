// Settings paths, JSON serialization, loading, saving, profile seeding.
// Included by main.cpp; keep this module focused on this responsibility.

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

void LoadConfig(Config& cfg) {
    std::error_code ec;
    std::filesystem::create_directories(UserDataDir(), ec);
    std::ifstream in(SettingsPath(), std::ios::binary);
    if (!in) return;
    try {
        json j; in >> j;
        cfg.endpoint = j.value("endpoint", cfg.endpoint);
        cfg.model = j.value("model", cfg.model);
        cfg.apiKey = j.value("api_key", cfg.apiKey);
        cfg.profileId = j.value("profile_id", cfg.profileId);
        cfg.computeMode = j.value("compute_mode", cfg.computeMode);
        cfg.showErrors = j.value("show_errors", cfg.showErrors);
        cfg.showWarnings = j.value("show_warnings", cfg.showWarnings);
        cfg.showContext = j.value("show_context", cfg.showContext);
        cfg.showKnownNoise = j.value("show_known_noise", cfg.showKnownNoise);
        cfg.showTimestamps = j.value("show_timestamps", cfg.showTimestamps);
        cfg.groupDiagnostics = j.value("group_diagnostics", cfg.groupDiagnostics);
        cfg.toastSeconds = j.value("toast_seconds", cfg.toastSeconds);
        cfg.toastFps = j.value("toast_fps", cfg.toastFps);
        cfg.toastSound = j.value("toast_sound", cfg.toastSound);
        cfg.startSoundPreset = j.value("start_sound_preset", cfg.startSoundPreset);
        cfg.endSoundPreset = j.value("end_sound_preset", cfg.endSoundPreset);
        cfg.offlineSoundPreset = j.value("offline_sound_preset", cfg.offlineSoundPreset);
        cfg.failureSoundPreset = j.value("failure_sound_preset", cfg.failureSoundPreset);
        cfg.toastSoundFile = j.value("toast_sound_file", cfg.toastSoundFile);
        cfg.toastShowType = j.value("toast_show_type", cfg.toastShowType);
        cfg.toastShowBytes = j.value("toast_show_bytes", cfg.toastShowBytes);
        cfg.toastShowTime = j.value("toast_show_time", cfg.toastShowTime);
        cfg.toastShowCounts = j.value("toast_show_counts", cfg.toastShowCounts);
        cfg.toastShowPreview = j.value("toast_show_preview", cfg.toastShowPreview);

        cfg.scanShowSource = j.value("popup_scan_show_source", cfg.toastShowType);
        cfg.scanShowModel = j.value("popup_scan_show_model", true);
        cfg.scanShowRoute = j.value("popup_scan_show_route", true);
        cfg.scanShowProgress = j.value("popup_scan_show_progress", true);
        cfg.scanShowPrefilterCounts = j.value("popup_scan_show_prefilter_counts", cfg.toastShowBytes);
        cfg.scanShowEstimatedTokens = j.value("popup_scan_show_estimated_tokens", cfg.toastShowBytes);
        cfg.scanShowBytesReduction = j.value("popup_scan_show_bytes_reduction", cfg.toastShowBytes);
        cfg.scanShowElapsedTime = j.value("popup_scan_show_elapsed_time", cfg.toastShowTime);

        cfg.resultShowDiagnosticTotal = j.value("popup_result_show_diagnostic_total", true);
        cfg.resultShowSource = j.value("popup_result_show_source", cfg.toastShowType);
        cfg.resultShowModel = j.value("popup_result_show_model", true);
        cfg.resultShowRoute = j.value("popup_result_show_route", true);
        cfg.resultShowFallbackNotice = j.value("popup_result_show_fallback_notice", true);
        cfg.resultShowPrefilterCounts = j.value("popup_result_show_prefilter_counts", cfg.toastShowBytes);
        cfg.resultShowEstimatedTokens = j.value("popup_result_show_estimated_tokens", cfg.toastShowBytes);
        cfg.resultShowRealTokens = j.value("popup_result_show_real_tokens", cfg.toastShowBytes);
        cfg.resultShowTokenSpeed = j.value("popup_result_show_token_speed", true);
        cfg.resultShowBytesReduction = j.value("popup_result_show_bytes_reduction", cfg.toastShowBytes);
        cfg.resultShowTime = j.value("popup_result_show_time", cfg.toastShowTime);
        cfg.resultShowAutoCopy = j.value("popup_result_show_auto_copy", true);
        cfg.resultShowCounts = j.value("popup_result_show_counts", cfg.toastShowCounts);
        cfg.resultShowPreview = j.value("popup_result_show_preview", cfg.toastShowPreview);
        cfg.resultShowLifetimeBar = j.value("popup_result_show_lifetime_bar", true);

        cfg.toastPreviewLines = std::clamp(j.value("toast_preview_lines", cfg.toastPreviewLines), 1, 10);
        cfg.toastAcknowledgeClipboard = j.value("toast_acknowledge_clipboard", cfg.toastAcknowledgeClipboard);
        cfg.autoCopyResults = j.value("auto_copy_results", cfg.autoCopyResults);
        cfg.watchClipboard = j.value("watch_clipboard", cfg.watchClipboard);
        cfg.ocrEnabled = j.value("ocr_enabled", cfg.ocrEnabled);
        cfg.autoScanImages = j.value("auto_scan_images", cfg.autoScanImages);
        cfg.ocrPromptSeconds = std::clamp(
            j.value("ocr_prompt_seconds", cfg.ocrPromptSeconds), 2.0f, 60.0f);
        cfg.ocrPromptShowImageDetails =
            j.value("ocr_prompt_show_image_details", cfg.ocrPromptShowImageDetails);
        cfg.ocrPromptShowTimeoutBar =
            j.value("ocr_prompt_show_timeout_bar", cfg.ocrPromptShowTimeoutBar);
        cfg.recentLimit = std::clamp(
            j.value("recent_limit", cfg.recentLimit), 1, 50);
        cfg.preferFastPath = j.value("prefer_fast_path", cfg.preferFastPath);

        auto loadColor = [&](const char* key, float (&dst)[3]) {
            if (!j.contains(key) || !j[key].is_array() || j[key].size() < 3) return;
            for (int i = 0; i < 3; ++i) dst[i] = j[key][i].get<float>();
        };
        loadColor("toast_bg", cfg.toastBg);
        loadColor("toast_accent", cfg.toastAccent);
    } catch (...) {
        // A malformed settings file should never prevent Log Sift from starting.
    }
}

void SaveConfig(const Config& cfg) {
    std::error_code ec;
    std::filesystem::create_directories(UserDataDir(), ec);
    std::ofstream out(SettingsPath(), std::ios::binary | std::ios::trunc);
    if (out) {
        out << ConfigToJson(cfg).dump(2) << '\n';
        out.close();
#ifndef _WIN32
        std::filesystem::permissions(SettingsPath(),
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace, ec);
#endif
    }
}

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

