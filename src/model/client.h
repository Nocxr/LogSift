#pragma once

#include "../config/config_types.h"
#include <string>
#include <vector>

struct ModelHealthResult {
    std::string status;
    bool online = false;
    bool modelAvailable = false;
    bool visionChecked = false;
    bool visionSupported = false;
    std::string visionDetail;
};

std::string ModelsEndpoint(const std::string& endpoint);
std::vector<std::string> ListModels(const Config& cfg);
std::string ApplyComputeMode(const Config& cfg);
ModelHealthResult CheckModel(const Config& cfg);
std::string SendRaw(const Config& cfg, const std::string& userText,
                    const std::string& systemText);
std::string Benchmark(const Config& cfg);
