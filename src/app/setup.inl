// Single-instance handling, SDL/ImGui setup, runtime state initialization.
// Included by main.cpp; keep this module focused on this responsibility.

#ifdef _WIN32
    gSingleInstanceMutex = CreateMutexW(
        nullptr, FALSE, L"Local\\Nocxr.LogSift.SingleInstance");
    if (gSingleInstanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        for (int attempt = 0; attempt < 40; ++attempt) {
            HWND existingTray = FindWindowExW(
                HWND_MESSAGE, nullptr, L"LogSiftTrayWindow", L"Log Sift Tray");
            if (existingTray) {
                PostMessageW(existingTray, kSingleInstanceMessage, 0, 0);
                break;
            }
            Sleep(50);
        }
        CloseHandle(gSingleInstanceMutex);
        gSingleInstanceMutex = nullptr;
        return 0;
    }
#elif defined(__APPLE__)
    if (LogSiftMacActivateExistingInstance())
        return 0;
#endif

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) return 1;
    // Keep the notification device alive for the whole process. This avoids
    // repeatedly opening/closing Bluetooth or USB headset paths for tiny UI sounds.
    EnsureNotificationAudio();
    SDL_WindowFlags mainWindowFlags = SDL_WINDOW_RESIZABLE;
    if (backgroundMode) mainWindowFlags |= SDL_WINDOW_HIDDEN;
    SDL_Window* window = SDL_CreateWindow("Log Sift", 1180, 800, mainWindowFlags);
    if (!window) return 1;
    SDL_SetWindowMinimumSize(window, 1100, 760);
#ifdef _WIN32
    gAppIconSmall = CreateLogSiftHIcon(32);
    gAppIconBig = CreateLogSiftHIcon(64);
    if (HWND hwnd = static_cast<HWND>(SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr))) {
        if (gAppIconSmall) SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(gAppIconSmall));
        if (gAppIconBig) SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(gAppIconBig));
    }
    InitTrayIcon();
#elif defined(__APPLE__)
    LogSiftMacTrayInit();
#endif
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) return 1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ApplyLogSiftStyle();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    ImGuiContext* mainContext = ImGui::GetCurrentContext();

    SDL_Window* toastWindow = SDL_CreateWindow("Log Sift Notification", 460, 230,
        SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_HIDDEN);
    SDL_Renderer* toastRenderer = toastWindow ? SDL_CreateRenderer(toastWindow, nullptr) : nullptr;
    ImGuiContext* toastContext = nullptr;
    if (toastWindow && toastRenderer) {
        toastContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(toastContext);
        ImGui::StyleColorsDark();
        ApplyLogSiftStyle();
        ImGui_ImplSDL3_InitForSDLRenderer(toastWindow, toastRenderer);
        ImGui_ImplSDLRenderer3_Init(toastRenderer);
        ImGui::SetCurrentContext(mainContext);
    }

    AppState app;
    auto& cfg = app.cfg;
    LoadConfig(cfg);
    if (!std::filesystem::exists(SettingsPath())) SaveConfig(cfg);
#ifdef _WIN32
    gTrayWatchEnabled = cfg.watchClipboard;
    gTrayOcrEnabled = cfg.ocrEnabled;
    gTrayAutoCopyEnabled = cfg.autoCopyResults;
    gTraySoundEnabled = cfg.toastSound;
    bool startAtLogin = WindowsGetStartAtLogin();
#elif defined(__APPLE__)
    LogSiftMacTraySetWatch(cfg.watchClipboard);
    LogSiftMacTraySetOcr(cfg.ocrEnabled);
    LogSiftMacTraySetAutoCopy(cfg.autoCopyResults);
    LogSiftMacTraySetSound(cfg.toastSound);
    bool startAtLogin = LogSiftMacGetStartAtLogin();
#else
    bool startAtLogin = false;
#endif
    app.lastSavedConfig = ConfigToJson(cfg).dump();
    app.recentRuns = LoadRecentRuns(cfg.recentLimit);
    app.recentCursor = app.recentRuns.empty() ? -1 : 0;
    app.selectedRecent = app.recentCursor;
#ifdef __APPLE__
    app.lastMacClipboardChangeCount = LogSiftMacClipboardChangeCount();
#endif
    auto& lastSavedConfig = app.lastSavedConfig;
    auto& input = app.input;
    auto& output = app.output;
    auto& questionableOutput = app.questionableOutput;
    auto& prompt = app.prompt;
    auto& status = app.status;
    auto& appLog = app.appLog;
    auto& copyFlash = app.copyFlash;
    auto& showAppLog = app.showAppLog;
    auto& lastClipboardText = app.lastClipboardText;
    auto& inputSourceKind = app.inputSourceKind;
#ifdef _WIN32
    auto& lastClipboardSequence = app.lastClipboardSequence;
#endif
#ifdef __APPLE__
    auto& lastMacClipboardChangeCount = app.lastMacClipboardChangeCount;
#endif
    auto& toastText = app.toastText;
    auto& toastUntil = app.toastUntil;
    auto& toastProcessing = app.toastProcessing;
    auto& toastSoundPlayed = app.toastSoundPlayed;
    auto& toastAutoCopied = app.toastAutoCopied;
    auto& toastStatsExpanded = app.toastStatsExpanded;
    auto& toastOcrStatsExpanded = app.toastOcrStatsExpanded;
    auto& toastPreviewExpanded = app.toastPreviewExpanded;
    auto& toastTimerPaused = app.popupTimer.paused;
    auto& toastResumeRequested = app.popupTimer.resumeRequested;
    auto& toastPausedRemaining = app.popupTimer.pausedRemaining;
    auto& toastOutcome = app.toastOutcome;
    auto& toastShownAt = app.toastShownAt;
    auto& request = app.request;
    auto& activeProgress = app.activeProgress;
    auto& requestGeneration = app.requestGeneration;
    auto& activeRequestGeneration = app.activeRequestGeneration;
    auto& healthRequest = app.healthRequest;
    auto& benchmarkRequest = app.benchmarkRequest;
    auto& modelListRequest = app.modelListRequest;
    auto& computeRequest = app.computeRequest;
    auto& busy = app.busy;
    auto& checkingHealth = app.checkingHealth;
    auto& benchmarking = app.benchmarking;
    auto& loadingModels = app.loadingModels;
    auto& applyingCompute = app.applyingCompute;
    auto& running = app.running;
    auto& availableModels = app.availableModels;
    auto& computeStatus = app.computeStatus;
    auto& stats = app.stats;
    auto& pendingOcrImage = app.pendingOcrImage;
    auto& pendingOcrImageReady = app.pendingOcrImageReady;
    auto& lastInputBytes = app.lastInputBytes;
    auto& lastFilteredBytes = app.lastFilteredBytes;
    auto& recentRuns = app.recentRuns;
    auto& recentCursor = app.recentCursor;
    auto& selectedRecent = app.selectedRecent;
    auto& connectionStage = app.connectionStage;
    auto& health = app.health;
    auto& visionSupportKnown = app.visionSupportKnown;
    auto& visionSupported = app.visionSupported;
    auto& visionStatus = app.visionStatus;
    auto& benchmarkStatus = app.benchmarkStatus;
    auto& startupConnectionSequence = app.startupConnectionSequence;
    auto& warnOnHealthFailure = app.warnOnHealthFailure;
    auto& requestStarted = app.requestStarted;
    auto& lastResponseSeconds = app.lastResponseSeconds;
    auto& lastClipboardCheck = app.lastClipboardCheck;
    AppendActivityLog(appLog, "INFO", "Log Sift started.");
    AppendActivityLog(appLog, "CONFIG", "Endpoint: " + cfg.endpoint + " | Model: " + cfg.model);
    AppendActivityLog(appLog, "CONFIG", std::string("Clipboard watch: ") + (cfg.watchClipboard ? "on" : "off"));
    AppendActivityLog(appLog, "CONFIG", std::string("OCR: ") + (cfg.ocrEnabled ? "on" : "off"));
    auto markOwnClipboardWrite = [&]() {
#ifdef _WIN32
        lastClipboardSequence = GetClipboardSequenceNumber();
        gClipboardUpdatePending = false;
#endif
    };
    auto applyRecentRun = [&](int index) {
        if (recentRuns.empty()) return;
        index = std::clamp(index, 0, static_cast<int>(recentRuns.size()) - 1);
        const RecentRun& r = recentRuns[static_cast<size_t>(index)];
        recentCursor = index;
        selectedRecent = index;
        input = r.input;
        output = r.output;
        questionableOutput = r.questionable;
        inputSourceKind = r.sourceKind.empty() ? "Manual" : r.sourceKind;
        stats = {};
        stats.sourceKind = inputSourceKind;
        stats.logType = r.logType;
        stats.profile = r.profile;
        stats.route = r.route;
        stats.model = r.model;
        stats.compute = r.compute;
        stats.inputBytes = r.inputBytes;
        stats.filteredBytes = r.filteredBytes;
        stats.inputLines = r.inputLines;
        stats.filteredLines = r.filteredLines;
        stats.inputWords = r.inputWords;
        stats.filteredWords = r.filteredWords;
        stats.estimatedInputTokens = r.estimatedInputTokens;
        stats.estimatedFilteredTokens = r.estimatedFilteredTokens;
        stats.seconds = r.seconds;
        stats.promptTokens = r.promptTokens;
        stats.completionTokens = r.completionTokens;
        stats.promptTokensPerSecond = r.promptTokensPerSecond;
        stats.completionTokensPerSecond = r.completionTokensPerSecond;
        stats.ocr = r.ocr;
        lastInputBytes = stats.inputBytes;
        lastFilteredBytes = stats.filteredBytes;
        status = "Loaded recent run from " + r.timestamp + ".";
    };

    auto cycleRecentRun = [&](int delta) {
        if (recentRuns.empty()) return;
        if (recentCursor < 0) recentCursor = 0;
        const int count = static_cast<int>(recentRuns.size());
        recentCursor = (recentCursor + delta) % count;
        if (recentCursor < 0) recentCursor += count;
        applyRecentRun(recentCursor);
    };

    auto recordRecentRun = [&]() {
        if (input.empty()) return;

        RecentRun r;
        r.timestamp = HistoryTimestamp();
        r.sourceKind = stats.sourceKind;
        r.logType = stats.logType;
        r.profile = stats.profile;
        r.route = stats.route;
        r.model = stats.model;
        r.compute = stats.compute;
        r.input = input;
        r.output = output;
        r.questionable = questionableOutput;
        r.inputBytes = stats.inputBytes;
        r.filteredBytes = stats.filteredBytes;
        r.inputLines = stats.inputLines;
        r.filteredLines = stats.filteredLines;
        r.inputWords = stats.inputWords;
        r.filteredWords = stats.filteredWords;
        r.estimatedInputTokens = stats.estimatedInputTokens;
        r.estimatedFilteredTokens = stats.estimatedFilteredTokens;
        r.seconds = stats.seconds;
        r.promptTokens = stats.promptTokens;
        r.completionTokens = stats.completionTokens;
        r.promptTokensPerSecond = stats.promptTokensPerSecond;
        r.completionTokensPerSecond = stats.completionTokensPerSecond;
        r.ocr = stats.ocr;

        // History is run-based, not content-deduplicated: repeating the same
        // input with another model/profile still counts as a distinct recent run.
        recentRuns.insert(recentRuns.begin(), std::move(r));

        if (static_cast<int>(recentRuns.size()) > cfg.recentLimit)
            recentRuns.resize(static_cast<size_t>(cfg.recentLimit));
        recentCursor = 0;
        selectedRecent = 0;
        SaveRecentRuns(recentRuns);
    };

    auto cancelActiveSift = [&](const char* source) {
        const bool active =
            busy || toastProcessing ||
            (activeProgress && !activeProgress->cancelled.load());
        if (!active) return;

        ++requestGeneration; // permanently invalidate any worker result
        activeRequestGeneration = requestGeneration;
        if (activeProgress)
            activeProgress->cancelled = true;

        busy = false;
        request = std::future<SiftResult>{};
        activeProgress.reset();

        stats.route = "Cancelled";
        stats.seconds = requestStarted.time_since_epoch().count() != 0
            ? std::chrono::duration<double>(
                std::chrono::steady_clock::now() - requestStarted).count()
            : 0.0;
        status = "Sift cancelled.";
        toastProcessing = false;
        toastAutoCopied = false;
        toastOutcome = ToastOutcome::Cancelled;
        toastText = "Cancelled";
        toastSoundPlayed = true;
        toastShownAt = std::chrono::steady_clock::now();
        toastUntil = toastShownAt + std::chrono::milliseconds(1800);

        AppendActivityLog(appLog, "CANCEL",
            std::string("Sift cancelled from ") + source + ".");
    };

    auto beginOcrScan = [&](ClipboardImage image, const std::string& sourceKind,
                            const std::string& activitySource) {
        pendingOcrImage = {};
        pendingOcrImageReady = false;

        output.clear();
        questionableOutput.clear();
        lastInputBytes = 0;
        lastFilteredBytes = 0;
        stats = {};
        inputSourceKind = sourceKind.empty() ? "OCR" : sourceKind;
        stats.sourceKind = inputSourceKind;
        stats.logType = "Image / OCR";
        stats.profile = "Vision OCR";
        stats.model = cfg.model;
        stats.compute =
            cfg.computeMode == 1 ? "GPU max" :
            cfg.computeMode == 2 ? "CPU" : "Auto";
        stats.inputBytes = image.bytes.size();
        stats.route = "Vision OCR";
        stats.ocr.present = true;
        stats.ocr.mimeType = image.mimeType;
        stats.ocr.imageBytes = image.bytes.size();

        if (!cfg.ocrEnabled) {
            status = "Image ignored - OCR is disabled.";
            AppendActivityLog(appLog, "OCR", status);
            return;
        }

        if (!visionSupportKnown || !visionSupported) {
            status = visionSupportKnown
                ? "Image ignored - selected model does not support Vision/OCR."
                : "Image ignored - Vision/OCR support has not been confirmed.";
            stats.route = visionSupportKnown ? "Vision unsupported" : "Vision unknown";
            toastText = "Vision unavailable";
            toastProcessing = false;
            toastOutcome = ToastOutcome::Failure;
            toastSoundPlayed = false;
            toastAutoCopied = false;
            toastShownAt = std::chrono::steady_clock::now();
            toastUntil = toastShownAt + std::chrono::milliseconds(
                static_cast<int>(cfg.toastSeconds * 1000.0f));
            AppendActivityLog(appLog, "OCR", "Image not sent: " + status);
            return;
        }

        ++requestGeneration;
        input.clear();
        requestStarted = std::chrono::steady_clock::now();
        toastText = "Reading image";
        toastProcessing = true;
        toastOutcome = ToastOutcome::Processing;
        toastSoundPlayed = false;
        toastAutoCopied = false;
        toastShownAt = requestStarted;
        toastUntil = requestStarted + std::chrono::hours(1);
        if (cfg.toastSound) PlaySynthPreset(cfg.startSoundPreset, true);

        const Config capturedCfg = cfg;
        const std::string capturedPrompt = prompt;
        activeRequestGeneration = requestGeneration;
        activeProgress = std::make_shared<SiftProgress>();
        activeProgress->total = 1;
        activeProgress->chunking = false;
        busy = true;
        status = activitySource + " - extracting text...";
        AppendActivityLog(appLog, "OCR",
            activitySource + " sent to " + cfg.model + " for Vision/OCR.");
        request = LaunchSiftTask(
            [capturedCfg,
             capturedImage = std::move(image),
             capturedPrompt,
             progress = activeProgress]() mutable {
                return SendClipboardImage(
                    capturedCfg, capturedImage, capturedPrompt, progress);
            });
    };

    auto startHealthCheck = [&](bool startupSequence, bool warnIfUnreachable) {
        if (checkingHealth) {
            AppendActivityLog(appLog, "HEALTH", "Health check already in progress.");
            return;
        }
        startupConnectionSequence = startupSequence;
        warnOnHealthFailure = warnIfUnreachable;
        connectionStage = ConnectionStage::Checking;
        health = "Checking model...";
        visionSupportKnown = false;
        visionSupported = false;
        visionStatus = "Checking...";
        if (startupSequence) benchmarkStatus = "Waiting for health check...";
        checkingHealth = true;
        AppendActivityLog(appLog, "HEALTH",
            std::string(startupSequence ? "Startup" : warnIfUnreachable ? "Window-open" : "Manual") +
            " model health check started.");
        const Config capturedCfg = cfg;
        healthRequest = LaunchBackgroundTask([capturedCfg] { return CheckModel(capturedCfg); });
    };

    auto reopenMainWindow = [&]() {
        SDL_ShowWindow(window);
        SDL_RaiseWindow(window);
        AppendActivityLog(appLog, "UI", "Main window opened; rechecking model health.");
        startHealthCheck(false, true);
    };

    // Fresh launch: verify connectivity, discover models, then benchmark automatically.
    startHealthCheck(true, false);

