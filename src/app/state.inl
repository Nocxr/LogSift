// Long-lived desktop state. The existing event and UI fragments bind local
// references to these fields while their handlers are moved out of main().
enum class ToastOutcome {
    Processing, OcrPrompt, Success, Empty, OfflineFallback,
    ModelFallback, Cancelled, Failure
};

enum class ConnectionStage {
    Checking, LoadingModels, Benchmarking, Ready,
    Unreachable, ModelsFailed, BenchmarkFailed
};

struct AppState {
    Config cfg;
    std::string lastSavedConfig;
    std::string input, output, questionableOutput;
    std::string prompt = kDefaultPrompt;
    std::string status = "Paste text or drop a log/image file.";
    std::string appLog;
    CopyFlashState copyFlash;
    bool showAppLog = false;
    std::string lastClipboardText;
    std::string inputSourceKind = "Manual";
#ifdef _WIN32
    DWORD lastClipboardSequence = 0;
#endif
    std::string toastText;
    std::chrono::steady_clock::time_point toastUntil{};
    bool toastProcessing = false;
    bool toastSoundPlayed = false;
    bool toastAutoCopied = false;
    bool toastStatsExpanded = true;
    bool toastOcrStatsExpanded = true;
    bool toastPreviewExpanded = true;
    PopupTimerState popupTimer;
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
    bool busy = false, checkingHealth = false, benchmarking = false;
    bool loadingModels = false, applyingCompute = false, running = true;
    std::vector<std::string> availableModels;
    std::string computeStatus = "Auto";
    RunStats stats;
    ClipboardImage pendingOcrImage;
    bool pendingOcrImageReady = false;
    size_t lastInputBytes = 0, lastFilteredBytes = 0;
    std::vector<RecentRun> recentRuns;
    int recentCursor = -1;
    int selectedRecent = -1;
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
    std::chrono::steady_clock::time_point lastClipboardCheck =
        std::chrono::steady_clock::now();
#ifdef __APPLE__
    long long lastMacClipboardChangeCount = 0;
#endif
};
