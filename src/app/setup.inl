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

    Config cfg;
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
    std::string lastSavedConfig = ConfigToJson(cfg).dump();

    std::string input, output, questionableOutput, prompt = kDefaultPrompt,
        status = "Paste text or drop a log/image file.";
    std::string appLog;
    CopyFlashState copyFlash;
    AppendActivityLog(appLog, "INFO", "Log Sift started.");
    AppendActivityLog(appLog, "CONFIG", "Endpoint: " + cfg.endpoint + " | Model: " + cfg.model);
    AppendActivityLog(appLog, "CONFIG", std::string("Clipboard watch: ") + (cfg.watchClipboard ? "on" : "off"));
    AppendActivityLog(appLog, "CONFIG", std::string("OCR: ") + (cfg.ocrEnabled ? "on" : "off"));
    bool showAppLog = false;
    std::string lastClipboardText;
    std::string inputSourceKind = "Manual";
#ifdef _WIN32
    DWORD lastClipboardSequence = 0;
#endif
    auto markOwnClipboardWrite = [&]() {
#ifdef _WIN32
        lastClipboardSequence = GetClipboardSequenceNumber();
        gClipboardUpdatePending = false;
#endif
    };
    std::string toastText;
    std::chrono::steady_clock::time_point toastUntil{};
    bool toastProcessing = false;
    bool toastSoundPlayed = false;
    bool toastAutoCopied = false;
    bool toastStatsExpanded = true;
    bool toastOcrStatsExpanded = true;
    bool toastPreviewExpanded = true;

    bool toastTimerPaused = false;
    bool toastMouseWasOver = false;
    bool toastResumeRequested = false;
    std::chrono::steady_clock::duration toastPausedRemaining{};
    std::chrono::steady_clock::time_point toastMouseLeftAt{};
    std::chrono::steady_clock::time_point toastPauseToastShownAt{};

    enum class ToastOutcome {
        Processing,
        OcrPrompt,
        Success,
        Empty,
        OfflineFallback,
        ModelFallback,
        Cancelled,
        Failure
    };
    ToastOutcome toastOutcome = ToastOutcome::Processing;
    std::chrono::steady_clock::time_point toastShownAt{};
    std::future<SiftResult> request;
    std::shared_ptr<SiftProgress> activeProgress;
    unsigned long long requestGeneration = 0;
    unsigned long long activeRequestGeneration = 0;
    std::future<ModelHealthResult> healthRequest;
    std::future<std::string> benchmarkRequest;
    std::future<std::vector<std::string>> modelListRequest;
    std::future<std::string> computeRequest;
    bool busy = false, checkingHealth = false, benchmarking = false, loadingModels = false, applyingCompute = false, running = true;
    std::vector<std::string> availableModels;
    std::string computeStatus = "Auto";
    RunStats stats;
    ClipboardImage pendingOcrImage;
    bool pendingOcrImageReady = false;
    size_t lastInputBytes = 0, lastFilteredBytes = 0;

    std::vector<RecentRun> recentRuns = LoadRecentRuns(cfg.recentLimit);
    int recentCursor = recentRuns.empty() ? -1 : 0;
    int selectedRecent = recentRuns.empty() ? -1 : 0;

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

    enum class ConnectionStage {
        Checking,
        LoadingModels,
        Benchmarking,
        Ready,
        Unreachable,
        ModelsFailed,
        BenchmarkFailed
    };
    ConnectionStage connectionStage = ConnectionStage::Checking;
    std::string health = "Checking model...";
    bool visionSupportKnown = false;
    bool visionSupported = false;
    std::string visionStatus = "Not checked";
    std::string benchmarkStatus = "Waiting for model check...";
    bool startupConnectionSequence = true;
    bool warnOnHealthFailure = false;
    std::chrono::steady_clock::time_point requestStarted{};
    double lastResponseSeconds = 0.0;
    auto lastClipboardCheck = std::chrono::steady_clock::now(); // non-Windows fallback only
#ifdef __APPLE__
    long long lastMacClipboardChangeCount = LogSiftMacClipboardChangeCount();
#endif

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
        healthRequest = std::async(std::launch::async,
            [capturedCfg] { return CheckModel(capturedCfg); });
    };

    auto reopenMainWindow = [&]() {
        SDL_ShowWindow(window);
        SDL_RaiseWindow(window);
        AppendActivityLog(appLog, "UI", "Main window opened; rechecking model health.");
        startHealthCheck(false, true);
    };

    // Fresh launch: verify connectivity, discover models, then benchmark automatically.
    startHealthCheck(true, false);

