// Model endpoint discovery, health checks, raw requests, benchmark.
// Included by main.cpp; keep this module focused on this responsibility.

std::string ModelsEndpoint(const std::string& endpoint) {
    const std::string suffix = "/chat/completions";
    if (endpoint.size() >= suffix.size() &&
        endpoint.compare(endpoint.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return endpoint.substr(0, endpoint.size() - suffix.size()) + "/models";
    }
    return endpoint;
}

std::vector<std::string> ListModels(const Config& cfg) {
    std::vector<std::string> models;
    std::string cmd = "curl -sS --fail-with-body --max-time 3 " + ShellQuote(ModelsEndpoint(cfg.endpoint));
    if (!cfg.apiKey.empty()) cmd += " -H " + ShellQuote("Authorization: Bearer " + cfg.apiKey);
    cmd += " 2>&1";
    const json response = json::parse(ReadPipe(cmd));
    if (response.contains("data") && response["data"].is_array()) {
        for (const auto& item : response["data"]) {
            const std::string id = item.value("id", "");
            if (!id.empty()) models.push_back(id);
        }
    }
    return models;
}

std::string ApplyComputeMode(const Config& cfg) {
    if (cfg.computeMode == 0) return "Auto - existing LM Studio load configuration";
    const std::string gpu = cfg.computeMode == 1 ? "max" : "off";
    try { ReadPipe("lms unload " + ShellQuote(cfg.model) + " 2>&1"); } catch (...) {}
    ReadPipe("lms load " + ShellQuote(cfg.model) + " --gpu " + gpu + " 2>&1");
    return cfg.computeMode == 1 ? "GPU max applied" : "CPU applied";
}

struct ModelHealthResult {
    std::string status;
    bool online = false;
    bool modelAvailable = false;
    bool visionChecked = false;
    bool visionSupported = false;
    std::string visionDetail;
};

ModelHealthResult CheckModel(const Config& cfg) {
    ModelHealthResult result;

    std::string cmd = "curl -sS --fail-with-body --max-time 3 " + ShellQuote(ModelsEndpoint(cfg.endpoint));
    if (!cfg.apiKey.empty()) cmd += " -H " + ShellQuote("Authorization: Bearer " + cfg.apiKey);
    cmd += " 2>&1";
    const json response = json::parse(ReadPipe(cmd));
    result.online = true;

    if (!response.contains("data") || !response["data"].is_array()) {
        result.status = "Online - model list unavailable";
        return result;
    }

    for (const auto& item : response["data"]) {
        if (item.value("id", "") == cfg.model) {
            result.modelAvailable = true;
            break;
        }
    }

    result.status = result.modelAvailable
        ? "Online - model available"
        : "Online - selected model not listed";

    if (!result.modelAvailable) return result;

    // Probe multimodal support instead of guessing from the model name. This uses
    // the same OpenAI-compatible image_url message format that clipboard OCR uses.
    static constexpr const char* kProbePng =
        "iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAIAAAD8GO2jAAAAKUlEQVR4nO3NMQEAAAjDMMC/52ECvlRA"
        "00nqs3m9AwAAAAAAAAAAgMMWx/EDPS4YA2MAAAAASUVORK5CYII=";
    json probeBody = {
        {"model", cfg.model},
        {"messages", json::array({
            {{"role", "user"}, {"content", json::array({
                {{"type", "text"}, {"text", "Reply exactly: OK"}},
                {{"type", "image_url"}, {"image_url", {
                    {"url", std::string("data:image/png;base64,") + kProbePng}
                }}}
            })}}
        })},
        {"temperature", 0},
        {"max_tokens", 8}
    };

    const auto temp = std::filesystem::temp_directory_path() /
        ("logsift-vision-probe-" + std::to_string(SDL_GetTicks()) + ".json");
    {
        std::ofstream out(temp, std::ios::binary);
        out << probeBody.dump();
    }

    std::string probeCmd = "curl -sS --fail-with-body --max-time 20 -X POST " +
        ShellQuote(cfg.endpoint) + " -H " + ShellQuote("Content-Type: application/json");
    if (!cfg.apiKey.empty())
        probeCmd += " -H " + ShellQuote("Authorization: Bearer " + cfg.apiKey);
    probeCmd += " --data-binary @" + ShellQuote(temp.string()) + " 2>&1";

    try {
        const json probeResponse = json::parse(ReadPipe(probeCmd));
        result.visionChecked = true;
        result.visionSupported =
            !probeResponse.contains("error") &&
            probeResponse.contains("choices") &&
            probeResponse["choices"].is_array() &&
            !probeResponse["choices"].empty();
        result.visionDetail = result.visionSupported
            ? "image input accepted"
            : "image input was not accepted";
    } catch (const std::exception& e) {
        // If the endpoint was reachable but rejected the image request, that is a
        // useful negative capability result. Connectivity failures stay unknown.
        result.visionChecked = !IsEndpointUnavailableError(e.what());
        result.visionSupported = false;
        result.visionDetail = result.visionChecked
            ? "image input rejected"
            : "vision probe could not reach the endpoint";
    }

    std::error_code ec;
    std::filesystem::remove(temp, ec);
    return result;
}

std::string SendRaw(const Config& cfg, const std::string& userText, const std::string& systemText) {
    json body = {
        {"model", cfg.model},
        {"messages", json::array({
            {{"role", "system"}, {"content", systemText}},
            {{"role", "user"}, {"content", userText}}
        })},
        {"temperature", 0},
        {"max_tokens", 1024}
    };

    const auto temp = std::filesystem::temp_directory_path() /
        ("logsift-" + std::to_string(SDL_GetTicks()) + ".json");
    { std::ofstream f(temp, std::ios::binary); f << body.dump(); }

    std::string cmd = "curl -sS --fail-with-body --max-time 120 -X POST " +
        ShellQuote(cfg.endpoint) + " -H " + ShellQuote("Content-Type: application/json");
    if (!cfg.apiKey.empty()) cmd += " -H " + ShellQuote("Authorization: Bearer " + cfg.apiKey);
    cmd += " --data-binary @" + ShellQuote(temp.string()) + " 2>&1";
    std::string raw;
    try { raw = ReadPipe(cmd); }
    catch (...) { std::error_code ec; std::filesystem::remove(temp, ec); throw; }
    std::error_code ec; std::filesystem::remove(temp, ec);
    return raw;
}

std::string Benchmark(const Config& cfg) {
    const auto start = std::chrono::steady_clock::now();
    const std::string raw = SendRaw(cfg, "Reply with exactly: OK", "Follow the user instruction exactly.");
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    const json response = json::parse(raw);
    int promptTokens = 0, completionTokens = 0;
    if (response.contains("usage")) {
        promptTokens = response["usage"].value("prompt_tokens", 0);
        completionTokens = response["usage"].value("completion_tokens", 0);
    }
    std::ostringstream result;
    result << "Benchmark: " << seconds << " s";
    if (promptTokens || completionTokens)
        result << " | prompt " << promptTokens << " tok | output " << completionTokens << " tok";
    return result.str();
}

bool LooksLikeGenericLog(const std::string& text) {
