// Persistent application configuration data.
// Included by main.cpp; keep this module focused on this responsibility.

struct Config {
    std::string endpoint = "http://127.0.0.1:1234/v1/chat/completions";
    std::string model = "google/gemma-4-e4b";
    std::string apiKey;
    std::string profileId = "auto";
    int computeMode = 0; // 0 Auto, 1 GPU, 2 CPU
    bool showErrors = true;
    bool showWarnings = true;
    bool showContext = true;
    bool showKnownNoise = false;
    bool showTimestamps = false;
    bool groupDiagnostics = true;
    float toastBg[3] = {0.075f, 0.078f, 0.09f};
    float toastAccent[3] = {0.25f, 0.58f, 0.95f};
    float toastSeconds = 8.0f;
    int toastFps = 120;
    bool toastSound = true;
    int startSoundPreset = 1;
    int endSoundPreset = 3;
    int offlineSoundPreset = 5;
    int failureSoundPreset = 4;
    std::string toastSoundFile;
    bool autoCopyResults = false;
    bool watchClipboard = true;
    bool ocrEnabled = true;
    bool autoScanImages = true;
    float ocrPromptSeconds = 8.0f;
    bool ocrPromptShowImageDetails = true;
    bool ocrPromptShowTimeoutBar = true;
    int recentLimit = 5;
    bool preferFastPath = true;
    // Legacy shared popup toggles are kept for settings migration.
    bool toastShowType = true;
    bool toastShowBytes = true;
    bool toastShowTime = true;
    bool toastShowCounts = true;
    bool toastShowPreview = true;

    bool scanShowSource = true;
    bool scanShowModel = true;
    bool scanShowRoute = true;
    bool scanShowProgress = true;
    bool scanShowPrefilterCounts = true;
    bool scanShowEstimatedTokens = true;
    bool scanShowBytesReduction = true;
    bool scanShowElapsedTime = true;

    bool resultShowDiagnosticTotal = true;
    bool resultShowSource = true;
    bool resultShowModel = true;
    bool resultShowRoute = true;
    bool resultShowFallbackNotice = true;
    bool resultShowPrefilterCounts = true;
    bool resultShowEstimatedTokens = true;
    bool resultShowRealTokens = true;
    bool resultShowTokenSpeed = true;
    bool resultShowBytesReduction = true;
    bool resultShowTime = true;
    bool resultShowAutoCopy = true;
    bool resultShowCounts = true;
    bool resultShowPreview = true;
    bool resultShowLifetimeBar = true;

    int toastPreviewLines = 3;
    bool toastAcknowledgeClipboard = false;
};

