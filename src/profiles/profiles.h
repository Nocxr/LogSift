#pragma once

#include "../config/config_types.h"
#include <filesystem>
#include <string>
#include <vector>

struct LogProfile {
    std::string id, name;
    std::vector<std::string> detect, highPriority, warnings, questionable, noise;
};
extern std::vector<LogProfile> gProfiles;
extern std::filesystem::path gProfilesDir;
bool ContainsAny(const std::string& line, const std::vector<std::string>& needles);
void LoadProfiles(const char* argv0);
const LogProfile* FindProfile(const std::string& id);
std::vector<const LogProfile*> DetectProfiles(const std::string& text, const Config& cfg);
const LogProfile* DetectProfile(const std::string& text, const Config& cfg);
