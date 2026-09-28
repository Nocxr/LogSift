#pragma once

#include "config_types.h"
#include <filesystem>
#include <nlohmann/json.hpp>

std::filesystem::path UserDataDir();
std::filesystem::path SettingsPath();
nlohmann::json ConfigToJson(const Config& cfg);
void LoadConfig(Config& cfg);
void SaveConfig(const Config& cfg);
void SeedUserProfiles(const char* argv0);
