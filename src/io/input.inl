// File/image loading and human-readable diagnostic entry parsing.
// Included by main.cpp; keep this module focused on this responsibility.

bool LoadFile(const char* path, std::string& input, std::string& status) {
    if (!path) return false;
    std::ifstream f(path, std::ios::binary);
    if (!f) { status = "Could not open dropped file."; return false; }
    std::ostringstream ss; ss << f.rdbuf();
    input = ss.str();
    status = "Loaded " + std::filesystem::path(path).filename().string() +
             " (" + std::to_string(input.size()) + " bytes)";
    return true;
}

bool LoadImageFile(const char* path, ClipboardImage& image, std::string& status) {
    if (!path) return false;
    const std::filesystem::path p(path);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (ext != ".png" && ext != ".jpg" && ext != ".jpeg")
        return false;

    std::ifstream f(p, std::ios::binary);
    if (!f) {
        status = "Could not open dropped image.";
        return false;
    }
    image.bytes.assign(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>());
    if (image.bytes.empty()) {
        status = "Dropped image was empty.";
        return false;
    }
    image.mimeType = ext == ".png" ? "image/png" : "image/jpeg";
    status = "Loaded image " + p.filename().string() +
        " (" + std::to_string(image.bytes.size()) + " bytes)";
    return true;
}
std::pair<size_t, size_t> HumanTextStats(const std::string& s) {
    if (s.empty()) return {0, 0};
    size_t lines = 1, words = 0;
    bool inWord = false;
    for (unsigned char ch : s) {
        if (ch == '\n') ++lines;
        const bool whitespace = std::isspace(ch) != 0;
        if (!whitespace && !inWord) ++words;
        inWord = !whitespace;
    }
    return {lines, words};
}

std::vector<std::string> DiagnosticEntries(const std::string& text) {
    std::vector<std::string> entries;
    std::istringstream stream(text);
    std::string line, current;

    auto isExplicitStart = [](const std::string& s) {
        return s.find(": Error:") != std::string::npos ||
               s.find(": Warning:") != std::string::npos ||
               s.find("Fatal error:") != std::string::npos ||
               s.find("Ensure condition failed") != std::string::npos ||
               s.find("Assertion failed") != std::string::npos ||
               s.find("Unhandled Exception") != std::string::npos ||
               s.rfind("FAILED:", 0) == 0 ||
               s.find("): error ") != std::string::npos ||
               s.find("): warning ") != std::string::npos ||
               s.find(": error:") != std::string::npos ||
               s.find(": warning:") != std::string::npos;
    };

    auto isContinuation = [](const std::string& s) {
        if (s.empty()) return false;
        if (std::isspace(static_cast<unsigned char>(s.front()))) return true;

        const auto startsWith = [&](const char* prefix) {
            return s.rfind(prefix, 0) == 0;
        };
        return startsWith("note:") ||
               startsWith("Note:") ||
               startsWith("help:") ||
               startsWith("Help:") ||
               startsWith("^") ||
               startsWith("~") ||
               startsWith("In file included from") ||
               startsWith("from ") ||
               startsWith("required from") ||
               startsWith("with [") ||
               startsWith("at ") ||
               startsWith("Caused by:");
    };

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        // Explicit compiler/runtime starts always begin a new diagnostic. For generic
        // output, each unindented top-level line is also its own entry; only clearly
        // subordinate/continuation lines stay attached to the previous entry.
        const bool newEntry =
            !current.empty() &&
            (isExplicitStart(line) || !isContinuation(line));

        if (newEntry) {
            entries.push_back(current);
            current.clear();
        }

        if (!current.empty()) current += '\n';
        current += line;
    }

    if (!current.empty()) entries.push_back(current);
    return entries;
}

