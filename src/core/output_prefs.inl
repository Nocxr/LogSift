// Output cleanup, timestamp handling, auto-copy preferences.
// Included by main.cpp; keep this module focused on this responsibility.

bool HasActionableOutput(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return false;
    const std::string trimmed = text.substr(first);
    return trimmed.rfind("NO_DIAGNOSTICS", 0) != 0;
}

bool LooksLikeTimestampToken(const std::string& token) {
    int digits = 0;
    bool timestampPunctuation = false;
    for (unsigned char ch : token) {
        if (std::isdigit(ch)) { ++digits; continue; }
        if (ch == '.' || ch == ':' || ch == '-' || ch == '/' || ch == 'T' || ch == 'Z' || ch == ' ')
            timestampPunctuation = true;
        else
            return false;
    }
    return digits >= 4 && timestampPunctuation;
}

bool IsAllDigits(const std::string& token) {
    return !token.empty() && std::all_of(token.begin(), token.end(),
        [](unsigned char ch) { return std::isdigit(ch) != 0; });
}

std::string StripTimestampPrefix(std::string line) {
    size_t pos = 0;
    bool strippedTimestamp = false;

    if (!line.empty() && line[0] == '[') {
        const size_t close = line.find(']');
        if (close != std::string::npos) {
            const std::string token = line.substr(1, close - 1);
            if (LooksLikeTimestampToken(token)) {
                pos = close + 1;
                strippedTimestamp = true;
            }
        }
    }

    if (!strippedTimestamp && line.size() >= 16 &&
        std::isdigit(static_cast<unsigned char>(line[0])) &&
        std::isdigit(static_cast<unsigned char>(line[1])) &&
        std::isdigit(static_cast<unsigned char>(line[2])) &&
        std::isdigit(static_cast<unsigned char>(line[3])) &&
        (line[4] == '-' || line[4] == '/' || line[4] == '.')) {
        const size_t colon = line.find(':', 8);
        if (colon != std::string::npos && colon < 24) {
            const size_t end = line.find(' ', colon);
            if (end != std::string::npos) {
                pos = end + 1;
                strippedTimestamp = true;
            }
        }
    }

    if (!strippedTimestamp) return line;

    while (pos < line.size() &&
           std::isspace(static_cast<unsigned char>(line[pos])))
        ++pos;

    // Unreal normally follows the timestamp with a frame/thread token such as
    // [585]. OCR can turn that into [S85], [O], etc. If a short bracket token
    // is immediately followed by a Log category, treat it as the same prefix.
    if (pos < line.size() && line[pos] == '[') {
        const size_t close2 = line.find(']', pos + 1);
        if (close2 != std::string::npos && close2 - pos <= 12) {
            size_t after = close2 + 1;
            while (after < line.size() &&
                   std::isspace(static_cast<unsigned char>(line[after])))
                ++after;
            if (line.compare(after, 3, "Log") == 0)
                pos = after;
        }
    }

    // Be extra tolerant of OCR punctuation/spacing around Unreal prefixes.
    // Once a real timestamp was removed, the category itself is the safest
    // canonical start for display when it appears near the front.
    const size_t logPos = line.find("Log", pos);
    if (logPos != std::string::npos && logPos < pos + 32)
        pos = logPos;

    while (pos < line.size() &&
           std::isspace(static_cast<unsigned char>(line[pos])))
        ++pos;
    return line.substr(pos);
}

std::string ApplyOutputPreferences(const std::string& text, const Config& cfg) {
    if (cfg.showTimestamps || text.empty()) return text;
    std::istringstream in(text);
    std::ostringstream out;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out << StripTimestampPrefix(std::move(line)) << '\n';
    }
    return out.str();
}

bool MaybeAutoCopyResult(const Config& cfg, const std::string& text, std::string& lastClipboardText) {
    if (!cfg.autoCopyResults || !HasActionableOutput(text)) return false;
    return SetOwnedClipboardText(text, &lastClipboardText);
}

