// Token estimates, run statistics, recents persistence.
// Included by main.cpp; keep this module focused on this responsibility.

size_t EstimateTokenCount(const std::string& text) {
    if (text.empty()) return 0;
    // Fast display estimate for logs/code before the model returns tokenizer usage.
    // Code/log punctuation is usually a little denser than prose, so ~3.7 bytes/token
    // is a better approximation here than a prose-oriented 4 bytes/token.
    return std::max<size_t>(1, static_cast<size_t>(
        std::ceil(static_cast<double>(text.size()) / 3.7)));
}

struct OcrPassStats {
    bool present = false;
    std::string mimeType;
    size_t imageBytes = 0;
    size_t outputBytes = 0;
    size_t outputLines = 0;
    size_t outputWords = 0;
    int promptTokens = 0;
    int completionTokens = 0;
    double promptTokensPerSecond = 0.0;
    double completionTokensPerSecond = 0.0;
    double seconds = 0.0;
};

struct RunStats {
    std::string sourceKind = "Manual";
    std::string logType = "Unknown";
    std::string route = "Not run";
    std::string profile = "Generic Log";
    std::string model;
    std::string compute = "Auto";
    size_t inputBytes = 0;
    size_t filteredBytes = 0;
    size_t inputLines = 0;
    size_t filteredLines = 0;
    size_t inputWords = 0;
    size_t filteredWords = 0;
    size_t estimatedInputTokens = 0;
    size_t estimatedFilteredTokens = 0;
    double seconds = 0.0;
    double ttftSeconds = 0.0;
    int promptTokens = 0;
    int completionTokens = 0;
    double promptTokensPerSecond = 0.0;
    double completionTokensPerSecond = 0.0;
    double estimatedPromptTokensPerSecond = 0.0;
    OcrPassStats ocr;
};

struct RecentRun {
    std::string timestamp;
    std::string sourceKind;
    std::string logType;
    std::string profile;
    std::string route;
    std::string model;
    std::string compute;
    std::string input;
    std::string output;
    std::string questionable;
    size_t inputBytes = 0;
    size_t filteredBytes = 0;
    size_t inputLines = 0;
    size_t filteredLines = 0;
    size_t inputWords = 0;
    size_t filteredWords = 0;
    size_t estimatedInputTokens = 0;
    size_t estimatedFilteredTokens = 0;
    double seconds = 0.0;
    int promptTokens = 0;
    int completionTokens = 0;
    double promptTokensPerSecond = 0.0;
    double completionTokensPerSecond = 0.0;
    OcrPassStats ocr;
};

std::filesystem::path RecentsPath() {
    return UserDataDir() / "recents.json";
}

std::string HistoryTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream out;
    out << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

json RecentRunToJson(const RecentRun& r) {
    return json{
        {"timestamp", r.timestamp},
        {"source_kind", r.sourceKind},
        {"log_type", r.logType},
        {"profile", r.profile},
        {"route", r.route},
        {"model", r.model},
        {"compute", r.compute},
        {"input", r.input},
        {"output", r.output},
        {"questionable", r.questionable},
        {"input_bytes", r.inputBytes},
        {"filtered_bytes", r.filteredBytes},
        {"input_lines", r.inputLines},
        {"filtered_lines", r.filteredLines},
        {"input_words", r.inputWords},
        {"filtered_words", r.filteredWords},
        {"estimated_input_tokens", r.estimatedInputTokens},
        {"estimated_filtered_tokens", r.estimatedFilteredTokens},
        {"seconds", r.seconds},
        {"prompt_tokens", r.promptTokens},
        {"completion_tokens", r.completionTokens},
        {"prompt_tokens_per_second", r.promptTokensPerSecond},
        {"completion_tokens_per_second", r.completionTokensPerSecond},
        {"ocr", {
            {"present", r.ocr.present},
            {"mime_type", r.ocr.mimeType},
            {"image_bytes", r.ocr.imageBytes},
            {"output_bytes", r.ocr.outputBytes},
            {"output_lines", r.ocr.outputLines},
            {"output_words", r.ocr.outputWords},
            {"prompt_tokens", r.ocr.promptTokens},
            {"completion_tokens", r.ocr.completionTokens},
            {"prompt_tokens_per_second", r.ocr.promptTokensPerSecond},
            {"completion_tokens_per_second", r.ocr.completionTokensPerSecond},
            {"seconds", r.ocr.seconds}
        }}
    };
}

RecentRun RecentRunFromJson(const json& j) {
    RecentRun r;
    r.timestamp = j.value("timestamp", "");
    r.sourceKind = j.value("source_kind", "Manual");
    r.logType = j.value("log_type", "Unknown");
    r.profile = j.value("profile", "Generic Log");
    r.route = j.value("route", "Unknown");
    r.model = j.value("model", "");
    r.compute = j.value("compute", "Auto");
    r.input = j.value("input", "");
    r.output = j.value("output", "");
    r.questionable = j.value("questionable", "");
    r.inputBytes = j.value("input_bytes", static_cast<size_t>(r.input.size()));
    r.filteredBytes = j.value("filtered_bytes", static_cast<size_t>(0));
    r.inputLines = j.value("input_lines", static_cast<size_t>(0));
    r.filteredLines = j.value("filtered_lines", static_cast<size_t>(0));
    r.inputWords = j.value("input_words", static_cast<size_t>(0));
    r.filteredWords = j.value("filtered_words", static_cast<size_t>(0));
    r.estimatedInputTokens = j.value("estimated_input_tokens", static_cast<size_t>(0));
    r.estimatedFilteredTokens = j.value("estimated_filtered_tokens", static_cast<size_t>(0));
    r.seconds = j.value("seconds", 0.0);
    r.promptTokens = j.value("prompt_tokens", 0);
    r.completionTokens = j.value("completion_tokens", 0);
    r.promptTokensPerSecond = j.value("prompt_tokens_per_second", 0.0);
    r.completionTokensPerSecond = j.value("completion_tokens_per_second", 0.0);
    if (j.contains("ocr") && j["ocr"].is_object()) {
        const auto& o = j["ocr"];
        r.ocr.present = o.value("present", false);
        r.ocr.mimeType = o.value("mime_type", "");
        r.ocr.imageBytes = o.value("image_bytes", static_cast<size_t>(0));
        r.ocr.outputBytes = o.value("output_bytes", static_cast<size_t>(0));
        r.ocr.outputLines = o.value("output_lines", static_cast<size_t>(0));
        r.ocr.outputWords = o.value("output_words", static_cast<size_t>(0));
        r.ocr.promptTokens = o.value("prompt_tokens", 0);
        r.ocr.completionTokens = o.value("completion_tokens", 0);
        r.ocr.promptTokensPerSecond = o.value("prompt_tokens_per_second", 0.0);
        r.ocr.completionTokensPerSecond = o.value("completion_tokens_per_second", 0.0);
        r.ocr.seconds = o.value("seconds", 0.0);
    }
    return r;
}

std::vector<RecentRun> LoadRecentRuns(int limit) {
    std::vector<RecentRun> out;
    std::ifstream in(RecentsPath(), std::ios::binary);
    if (!in) return out;
    try {
        json root;
        in >> root;
        if (!root.is_array()) return out;
        for (const auto& item : root) {
            if (!item.is_object()) continue;
            out.push_back(RecentRunFromJson(item));
            if (static_cast<int>(out.size()) >= limit) break;
        }
    } catch (...) {}
    return out;
}

void SaveRecentRuns(const std::vector<RecentRun>& runs) {
    std::error_code ec;
    std::filesystem::create_directories(UserDataDir(), ec);
    json root = json::array();
    for (const auto& run : runs) root.push_back(RecentRunToJson(run));
    std::ofstream out(RecentsPath(), std::ios::binary);
    if (out) out << root.dump(2);
}

