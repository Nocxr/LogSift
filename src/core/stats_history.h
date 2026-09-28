#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

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

size_t EstimateTokenCount(const std::string& text);
std::filesystem::path RecentsPath();
std::string HistoryTimestamp();
nlohmann::json RecentRunToJson(const RecentRun& r);
RecentRun RecentRunFromJson(const nlohmann::json& j);
std::vector<RecentRun> LoadRecentRunsFile(const std::filesystem::path& path, int limit);
std::vector<RecentRun> LoadRecentRuns(int limit);
void SaveRecentRunsFile(const std::filesystem::path& path,
                        const std::vector<RecentRun>& runs);
void SaveRecentRuns(const std::vector<RecentRun>& runs);
