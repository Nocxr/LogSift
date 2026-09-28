// Shell/process helpers, formatting, diagnostic deduplication.
// Included by main.cpp; keep this module focused on this responsibility.

std::string ShellQuote(const std::string& s) {
#ifdef _WIN32
    std::string out = "\"";
    for (char c : s) out += (c == '"') ? "\\\"" : std::string(1, c);
    return out + "\"";
#else
    std::string out = "'";
    for (char c : s) out += (c == '\'') ? "'\\''" : std::string(1, c);
    return out + "'";
#endif
}

std::string Base64Encode(const std::vector<unsigned char>& data) {
    static constexpr char kTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i + 2 < data.size()) {
        const unsigned value =
            (static_cast<unsigned>(data[i]) << 16) |
            (static_cast<unsigned>(data[i + 1]) << 8) |
            static_cast<unsigned>(data[i + 2]);
        out.push_back(kTable[(value >> 18) & 0x3F]);
        out.push_back(kTable[(value >> 12) & 0x3F]);
        out.push_back(kTable[(value >> 6) & 0x3F]);
        out.push_back(kTable[value & 0x3F]);
        i += 3;
    }

    if (i < data.size()) {
        unsigned value = static_cast<unsigned>(data[i]) << 16;
        out.push_back(kTable[(value >> 18) & 0x3F]);
        if (i + 1 < data.size()) {
            value |= static_cast<unsigned>(data[i + 1]) << 8;
            out.push_back(kTable[(value >> 12) & 0x3F]);
            out.push_back(kTable[(value >> 6) & 0x3F]);
            out.push_back('=');
        } else {
            out.push_back(kTable[(value >> 12) & 0x3F]);
            out.push_back('=');
            out.push_back('=');
        }
    }
    return out;
}

bool IsEndpointUnavailableError(const std::string& message) {
    std::string lower = message;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    return lower.find("curl failed (exit 6)") != std::string::npos ||
           lower.find("curl failed (exit 7)") != std::string::npos ||
           lower.find("curl failed (exit 28)") != std::string::npos ||
           lower.find("could not resolve") != std::string::npos ||
           lower.find("failed to connect") != std::string::npos ||
           lower.find("connection refused") != std::string::npos ||
           lower.find("could not connect") != std::string::npos ||
           lower.find("timeout was reached") != std::string::npos ||
           lower.find("timed out") != std::string::npos ||
           lower.find("could not start curl") != std::string::npos;
}

std::string ReadPipe(const std::string& command) {
#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (!pipe) throw std::runtime_error("Could not start curl.");
    std::array<char, 4096> buf{};
    std::string out;
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) out += buf.data();
#ifdef _WIN32
    const int code = _pclose(pipe);
#else
    const int code = pclose(pipe);
#endif
    if (code != 0) throw std::runtime_error("curl failed (exit " + std::to_string(code) + "):\n" + out);
    return out;
}

std::string DedupeLines(const std::string& text);

std::string FormatDiagnosticText(const std::string& text, const Config& cfg) {
    if (cfg.showTimestamps) return DedupeLines(text);

    std::istringstream in(text);
    std::ostringstream out;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string shown = line;

        // Unreal prefixes commonly look like:
        // [2026.09.26-03.21.13:141][585]LogCategory: ...
        // Strip the timestamp/frame prefix while keeping the category and message.
        if (!shown.empty() && shown.front() == '[') {
            const size_t logPos = shown.find("Log");
            if (logPos != std::string::npos && logPos < 96) {
                shown = shown.substr(logPos);
            } else {
                const size_t close = shown.find(']');
                if (close != std::string::npos && close < 64) {
                    const std::string prefix = shown.substr(1, close - 1);
                    const bool timestampish =
                        prefix.find(':') != std::string::npos ||
                        prefix.find('.') != std::string::npos ||
                        prefix.find('-') != std::string::npos;
                    const bool hasDigit = std::any_of(prefix.begin(), prefix.end(),
                        [](unsigned char ch) { return std::isdigit(ch) != 0; });
                    if (timestampish && hasDigit) {
                        size_t start = close + 1;
                        // Strip one additional numeric frame/thread bracket.
                        if (start < shown.size() && shown[start] == '[') {
                            const size_t close2 = shown.find(']', start);
                            if (close2 != std::string::npos && close2 - start < 16)
                                start = close2 + 1;
                        }
                        while (start < shown.size() && std::isspace(static_cast<unsigned char>(shown[start]))) ++start;
                        shown = shown.substr(start);
                    }
                }
            }
        }
        out << shown << '\n';
    }
    return DedupeLines(out.str());
}

std::string DedupeLines(const std::string& text) {
    std::istringstream in(text);
    std::ostringstream out;
    std::unordered_set<std::string> seen;
    std::string line;
    bool lastBlank = false;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const bool blank = line.find_first_not_of(" \t") == std::string::npos;
        if (blank) {
            if (!lastBlank) out << '\n';
            lastBlank = true;
            continue;
        }
        lastBlank = false;
        if (seen.insert(line).second) out << line << '\n';
    }
    return out.str();
}

