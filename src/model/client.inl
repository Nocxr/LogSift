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
    const json response = json::parse(HttpGet(ModelsEndpoint(cfg.endpoint), cfg.apiKey, 3));
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
    if (cfg.model.empty() || !std::all_of(cfg.model.begin(), cfg.model.end(), [](char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') || c == '/' || c == '.' ||
                   c == '_' || c == '-' || c == ':';
        }))
        throw std::runtime_error("Model ID has unsupported characters for the local lms command.");
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

    const json response = json::parse(HttpGet(ModelsEndpoint(cfg.endpoint), cfg.apiKey, 3));
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

    try {
        const json probeResponse = json::parse(HttpPostJson(cfg.endpoint, cfg.apiKey, probeBody.dump(), 20));
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

    return HttpPostJson(cfg.endpoint, cfg.apiKey, body.dump(), 120);
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
