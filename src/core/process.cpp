#include "process.h"
#include "../config/config_types.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#ifdef _WIN32
#include <windows.h>
#endif

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
    SECURITY_ATTRIBUTES security{};
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &security, 0))
        throw std::runtime_error("Could not create process output pipe.");
    if (!SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(readPipe);
        CloseHandle(writePipe);
        throw std::runtime_error("Could not configure process output pipe.");
    }

    HANDLE nullInput = CreateFileA(
        "NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nullInput == INVALID_HANDLE_VALUE) nullInput = nullptr;

    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    startup.hStdInput = nullInput;
    startup.hStdOutput = writePipe;
    startup.hStdError = writePipe;

    PROCESS_INFORMATION process{};
    std::string commandLine = "cmd.exe /D /S /C \"" + command + "\"";
    const BOOL started = CreateProcessA(
        nullptr, commandLine.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);

    CloseHandle(writePipe);
    if (nullInput) CloseHandle(nullInput);

    if (!started) {
        const DWORD error = GetLastError();
        CloseHandle(readPipe);
        throw std::runtime_error(
            "Could not start process (Windows error " + std::to_string(error) + ").");
    }

    std::array<char, 4096> buf{};
    std::string out;
    DWORD bytesRead = 0;
    while (ReadFile(readPipe, buf.data(), static_cast<DWORD>(buf.size()), &bytesRead, nullptr) &&
           bytesRead > 0) {
        out.append(buf.data(), bytesRead);
    }
    CloseHandle(readPipe);

    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    if (exitCode != 0)
        throw std::runtime_error(
            "process failed (exit " + std::to_string(exitCode) + "):\n" + out);
    return out;
#else
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("Could not start process.");
    std::array<char, 4096> buf{};
    std::string out;
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) out += buf.data();
    const int code = pclose(pipe);
    if (code != 0)
        throw std::runtime_error(
            "process failed (exit " + std::to_string(code) + "):\n" + out);
    return out;
#endif
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

