// Main window frame setup and model/connection controls.
// Included by main.cpp; keep this module focused on this responsibility.

        const bool mainVisibleForFrame =
            (SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) == 0;
        // Keep this deliberately simple: if the main window is visible, render it
        // every frame at 60 FPS. If it is hidden, never build or present a main UI frame.
        if (mainVisibleForFrame) {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({0,0});
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Log Sift", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SeparatorText("MODEL / CONNECTION");

        const char* connectionLabel = "Ready";
        ImVec4 connectionColor(0.30f, 0.90f, 0.48f, 1.0f);
        switch (connectionStage) {
            case ConnectionStage::Checking:
                connectionLabel = "Checking model...";
                connectionColor = ImVec4(0.95f, 0.78f, 0.28f, 1.0f);
                break;
            case ConnectionStage::LoadingModels:
                connectionLabel = "Loading models...";
                connectionColor = ImVec4(0.30f, 0.72f, 1.00f, 1.0f);
                break;
            case ConnectionStage::Benchmarking:
                connectionLabel = "Benchmarking...";
                connectionColor = ImVec4(0.72f, 0.48f, 1.00f, 1.0f);
                break;
            case ConnectionStage::Ready:
                connectionLabel = "Ready";
                connectionColor = ImVec4(0.30f, 0.90f, 0.48f, 1.0f);
                break;
            case ConnectionStage::Unreachable:
                connectionLabel = "UNREACHABLE";
                connectionColor = ImVec4(1.00f, 0.30f, 0.28f, 1.0f);
                break;
            case ConnectionStage::ModelsFailed:
                connectionLabel = "Online / model list unavailable";
                connectionColor = ImVec4(0.95f, 0.62f, 0.22f, 1.0f);
                break;
            case ConnectionStage::BenchmarkFailed:
                connectionLabel = "Online / benchmark failed";
                connectionColor = ImVec4(0.95f, 0.62f, 0.22f, 1.0f);
                break;
        }

        ImVec4 visionColor(0.58f, 0.60f, 0.66f, 1.0f);
        if (visionSupportKnown && visionSupported)
            visionColor = ImVec4(0.25f, 0.88f, 0.78f, 1.0f);
        else if (visionSupportKnown && !visionSupported)
            visionColor = ImVec4(0.95f, 0.58f, 0.22f, 1.0f);
        else if (checkingHealth)
            visionColor = ImVec4(0.35f, 0.72f, 1.00f, 1.0f);

        const std::string visionPill =
            "OCR / VISION  " +
            std::string(
                !visionSupportKnown
                    ? (checkingHealth ? "CHECKING" : "UNKNOWN")
                    : visionSupported ? "SUPPORTED" : "NOT SUPPORTED");

        if (ImGui::BeginTable(
                "##model_connection_table", 2,
                ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_BordersInnerH)) {
            ImGui::TableSetupColumn(
                "##connection_label", ImGuiTableColumnFlags_WidthFixed, 86.0f);
            ImGui::TableSetupColumn(
                "##connection_value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(
                ImVec4(0.42f, 0.78f, 1.00f, 1.0f), "Endpoint");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##endpoint", &cfg.endpoint);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(
                ImVec4(0.72f, 0.62f, 1.00f, 1.0f), "Model");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(340);
            if (!availableModels.empty()) {
                if (ImGui::BeginCombo("##modelcombo", cfg.model.c_str())) {
                    for (const auto& m : availableModels) {
                        const bool selected = m == cfg.model;
                        if (ImGui::Selectable(m.c_str(), selected)) {
                            if (cfg.model != m) {
                                cfg.model = m;
                                startHealthCheck(false, false);
                            }
                        }
                        if (selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            } else {
                ImGui::InputText("##model", &cfg.model);
            }
            ImGui::SameLine();
            if (ImGui::Button(
                    loadingModels ? "Refreshing..." : "Refresh Models")) {
                const Config capturedCfg = cfg;
                loadingModels = true;
                AppendActivityLog(
                    appLog, "MODELS", "Manual model refresh started.");
                startupConnectionSequence = false;
                connectionStage = ConnectionStage::LoadingModels;
                modelListRequest = std::async(
                    std::launch::async,
                    [capturedCfg] { return ListModels(capturedCfg); });
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("API key");
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText(
                "##key", &cfg.apiKey, ImGuiInputTextFlags_Password);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Actions");
            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button("LM Studio")) {
                cfg.endpoint =
                    "http://127.0.0.1:1234/v1/chat/completions";
                cfg.model = "google/gemma-4-e4b";
                cfg.apiKey.clear();
                health = "Not checked";
                visionSupportKnown = false;
                visionSupported = false;
                visionStatus = "Not checked";
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(
                checkingHealth || cfg.endpoint.empty());
            if (ImGui::Button(
                    checkingHealth ? "Checking..." : "Check Model")) {
                startHealthCheck(false, true);
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(
                benchmarking || cfg.endpoint.empty() || cfg.model.empty());
            if (ImGui::Button(
                    benchmarking ? "Benchmarking..." : "Benchmark")) {
                const Config capturedCfg = cfg;
                benchmarking = true;
                AppendActivityLog(
                    appLog, "BENCH",
                    "Manual benchmark started for " + cfg.model + ".");
                startupConnectionSequence = false;
                connectionStage = ConnectionStage::Benchmarking;
                benchmarkStatus = "Benchmarking...";
                benchmarkRequest = std::async(
                    std::launch::async,
                    [capturedCfg] { return Benchmark(capturedCfg); });
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Reset Prompt"))
                prompt = kDefaultPrompt;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Status");
            ImGui::TableSetColumnIndex(1);
            DrawStatusPill(connectionLabel, connectionColor);
            ImGui::SameLine();
            DrawStatusPill(visionPill.c_str(), visionColor);
            ImGui::SameLine();
            ImGui::TextDisabled("%s", health.c_str());
            if (lastResponseSeconds > 0.0) {
                ImGui::SameLine();
                ImGui::TextColored(
                    ImVec4(0.55f, 0.82f, 1.0f, 1.0f),
                    "Last %.3f s", lastResponseSeconds);
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Runtime");
            ImGui::TableSetColumnIndex(1);
            const char* computeItems[] = {"Auto", "GPU max", "CPU"};
            ImGui::SetNextItemWidth(120);
            ImGui::Combo(
                "##compute", &cfg.computeMode, computeItems, 3);
            ImGui::SameLine();
            ImGui::BeginDisabled(
                applyingCompute || cfg.model.empty() ||
                cfg.computeMode == 0);
            if (ImGui::Button(
                    applyingCompute ? "Applying..." : "Apply Compute")) {
                const Config capturedCfg = cfg;
                applyingCompute = true;
                computeRequest = std::async(
                    std::launch::async,
                    [capturedCfg] {
                        return ApplyComputeMode(capturedCfg);
                    });
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::TextDisabled("%s", computeStatus.c_str());
            if (!benchmarkStatus.empty()) {
                ImGui::SameLine();
                ImGui::TextDisabled("| %s", benchmarkStatus.c_str());
            }
            if (!status.empty()) {
                ImGui::SameLine();
                ImGui::TextDisabled("| %s", status.c_str());
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(
            ImVec4(0.42f, 0.78f, 1.00f, 1.0f),
            "WORKSPACE");
        ImGui::SameLine();
        const std::string recentWorkspacePill =
            "RECENTS  " + std::to_string(recentRuns.size());
        DrawStatusPill(
            recentWorkspacePill.c_str(),
            recentRuns.empty()
                ? ImVec4(0.50f, 0.54f, 0.60f, 1.0f)
                : ImVec4(0.72f, 0.62f, 1.00f, 1.0f));
        ImGui::SameLine();
        ImGui::TextDisabled(
            "Sift inputs and results  /  browse recent runs  /  settings");
        ImGui::Separator();
        ImGui::Spacing();

