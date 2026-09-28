// Sift tab input, controls, and result panes.
// Included by main.cpp; keep this module focused on this responsibility.

        if (ImGui::BeginTabBar("##main_tabs")) {
            if (ImGui::BeginTabItem("Sift")) {
        ImGui::SeparatorText("SOURCE / AUTOMATION");
        ImGui::TextColored(
            ImVec4(0.72f, 0.62f, 1.00f, 1.0f), "Profile");
        ImGui::SameLine();
        std::vector<std::string> profileLabels{"Auto","Generic"};
        std::vector<std::string> profileIds{"auto","generic"};
        for (const auto& p : gProfiles) {
            if (p.id != "generic") {
                profileLabels.push_back(p.name);
                profileIds.push_back(p.id);
            }
        }
        int profileIndex = 0;
        for (size_t i = 0; i < profileIds.size(); ++i) {
            if (profileIds[i] == cfg.profileId)
                profileIndex = static_cast<int>(i);
        }
        std::vector<const char*> profileItems;
        for (auto& s : profileLabels)
            profileItems.push_back(s.c_str());
        ImGui::SetNextItemWidth(180);
        if (ImGui::Combo(
                "##profile", &profileIndex,
                profileItems.data(),
                static_cast<int>(profileItems.size()))) {
            cfg.profileId = profileIds[static_cast<size_t>(profileIndex)];
        }
        ImGui::SameLine();
        ImGui::TextDisabled(
            "Detected: %s",
            input.empty() ? "-" : ProfileName(input, cfg).c_str());

        if (ImGui::BeginTable(
                "##automation_controls", 3,
                ImGuiTableFlags_SizingStretchSame |
                ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableNextColumn();
            ImGui::TextColored(
                ImVec4(0.42f, 0.78f, 1.00f, 1.0f),
                "CLIPBOARD");
            if (ImGui::Checkbox(
                    "Watch clipboard##automation",
                    &cfg.watchClipboard)) {
#ifdef _WIN32
                gTrayWatchEnabled = cfg.watchClipboard;
#elif defined(__APPLE__)
                LogSiftMacTraySetWatch(cfg.watchClipboard);
#endif
                lastClipboardText.clear();
#ifdef _WIN32
                lastClipboardSequence = 0;
#endif
                status = cfg.watchClipboard
                    ? "Clipboard watch enabled."
                    : "Clipboard watch disabled.";
                AppendActivityLog(appLog, "WATCH", status);
            }
            ImGui::TextDisabled(
                "Sift copied logs automatically.");

            ImGui::TableNextColumn();
            ImGui::TextColored(
                ImVec4(0.25f, 0.88f, 0.78f, 1.0f),
                "OCR / IMAGES");
            if (ImGui::Checkbox(
                    "OCR images##automation",
                    &cfg.ocrEnabled)) {
#ifdef _WIN32
                gTrayOcrEnabled = cfg.ocrEnabled;
#elif defined(__APPLE__)
                LogSiftMacTraySetOcr(cfg.ocrEnabled);
#endif
                status = cfg.ocrEnabled
                    ? "OCR enabled."
                    : "OCR disabled.";
                AppendActivityLog(appLog, "OCR", status);
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(!cfg.ocrEnabled);
            ImGui::Checkbox(
                "Auto-scan##images", &cfg.autoScanImages);
            ImGui::EndDisabled();
            ImGui::TextDisabled(
                cfg.autoScanImages
                    ? "Clipboard images scan immediately."
                    : "Clipboard images ask first.");

            ImGui::TableNextColumn();
            ImGui::TextColored(
                ImVec4(1.00f, 0.82f, 0.25f, 1.0f),
                "FAST PATH");
            ImGui::Checkbox(
                "Structured compiler logs##automation",
                &cfg.preferFastPath);
            ImGui::TextDisabled(
                "Skip the model when extraction is deterministic.");

            ImGui::EndTable();
        }

        ImGui::SeparatorText("FILTER OUTPUT");
        ImGui::TextDisabled("Include");
        ImGui::SameLine();
        ColoredCheckbox("Errors / Fatal", &cfg.showErrors,
            ImVec4(1.00f, 0.38f, 0.38f, 1.0f));
        ImGui::SameLine();
        ColoredCheckbox("Warnings", &cfg.showWarnings,
            ImVec4(1.00f, 0.82f, 0.25f, 1.0f));
        ImGui::SameLine();
        ColoredCheckbox("Notes / Context", &cfg.showContext,
            ImVec4(0.42f, 0.78f, 1.00f, 1.0f));
        ImGui::SameLine();
        ColoredCheckbox("Known noise", &cfg.showKnownNoise,
            ImVec4(0.62f, 0.64f, 0.70f, 1.0f));
        ImGui::SameLine();
        ColoredCheckbox("Timestamps", &cfg.showTimestamps,
            ImVec4(0.72f, 0.62f, 1.00f, 1.0f));
        ImGui::SameLine();
        ColoredCheckbox("Group", &cfg.groupDiagnostics,
            ImVec4(0.38f, 0.88f, 0.56f, 1.0f));

        if (ImGui::CollapsingHeader("Advanced / system prompt")) {
            ImGui::TextDisabled(
                "Used only when the model path is needed; deterministic fast-path runs ignore it.");
            ImGui::InputTextMultiline("##prompt", &prompt, {-1, 120});
        }

        ImGui::SeparatorText("RUN / STATS");
        const double reduction = stats.inputBytes
            ? 100.0 * (1.0 - static_cast<double>(stats.filteredBytes) /
                static_cast<double>(stats.inputBytes))
            : 0.0;
        ImGui::TextColored(ImVec4(0.45f,0.75f,1.0f,1.0f),
            "Source: %s   Type: %s   Profile: %s   Route: %s   Compute: %s   Health: %s",
            stats.sourceKind.c_str(), stats.logType.c_str(), stats.profile.c_str(), stats.route.c_str(),
            cfg.computeMode == 1 ? "GPU max" : cfg.computeMode == 2 ? "CPU" : "Auto",
            health.c_str());
        ImGui::TextColored(ImVec4(0.45f,1.0f,0.55f,1.0f),
            "Input: %zu -> %zu bytes   Reduction: %.1f%%   Last: %.3f s   Prompt: %d tok   Output: %d tok",
            stats.inputBytes, stats.filteredBytes, reduction, stats.seconds,
            stats.promptTokens, stats.completionTokens);
        if (stats.ocr.present) {
            ImGui::SeparatorText("OCR PASS");
            ImGui::TextColored(ImVec4(0.72f,0.62f,1.0f,1.0f),
                "Image: %s   %zu bytes",
                stats.ocr.mimeType.empty() ? "image" : stats.ocr.mimeType.c_str(),
                stats.ocr.imageBytes);
            ImGui::TextDisabled(
                "Extracted: %zu bytes   %zu lines   %zu words",
                stats.ocr.outputBytes, stats.ocr.outputLines, stats.ocr.outputWords);
            ImGui::TextColored(ImVec4(0.45f,1.0f,0.55f,1.0f),
                "Time: %.3f s   Prompt: %d tok   Output: %d tok",
                stats.ocr.seconds, stats.ocr.promptTokens, stats.ocr.completionTokens);
            if (stats.ocr.promptTokensPerSecond > 0.0 ||
                stats.ocr.completionTokensPerSecond > 0.0) {
                ImGui::SameLine();
                ImGui::TextDisabled("%.0f prompt/s | %.0f output/s",
                    stats.ocr.promptTokensPerSecond,
                    stats.ocr.completionTokensPerSecond);
            }
        }

        const float workAreaHeight = std::max(260.0f, ImGui::GetContentRegionAvail().y - 2.0f);
        if (ImGui::BeginTable("##sift_work_area", 2,
            ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_SizingStretchProp,
            ImVec2(-1.0f, workAreaHeight))) {

            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch, 0.47f);
            ImGui::TableSetupColumn("Results", ImGuiTableColumnFlags_WidthStretch, 0.53f);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::BeginChild("##input_pane", ImVec2(0, 0), ImGuiChildFlags_None,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::SeparatorText("INPUT");
            const auto [inputLines, inputWords] = HumanTextStats(input);
            ImGui::TextDisabled("%zu lines  |  %zu words  |  %zu bytes",
                inputLines, inputWords, input.size());
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear Input")) {
                input.clear();
                output.clear();
                questionableOutput.clear();
                lastInputBytes = lastFilteredBytes = 0;
                stats = {};
                inputSourceKind = "Manual";
                stats.sourceKind = inputSourceKind;
                recentCursor = -1;
                status = "Input cleared.";
            }

            if (!recentRuns.empty()) {
                ImGui::SameLine();
                ImGui::TextDisabled("Recent");
                ImGui::SameLine();
                if (ImGui::SmallButton("< Older"))
                    cycleRecentRun(1);
                ImGui::SameLine();
                ImGui::TextDisabled("%d / %zu",
                    recentCursor >= 0 ? recentCursor + 1 : 0,
                    recentRuns.size());
                ImGui::SameLine();
                if (ImGui::SmallButton("Newer >"))
                    cycleRecentRun(-1);
            }

            const float paneActionReserve =
                ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
            const float inputEditorHeight = std::max(
                120.0f, ImGui::GetContentRegionAvail().y - paneActionReserve);
            if (ImGui::InputTextMultiline(
                    "##input", &input, {-1, inputEditorHeight})) {
                inputSourceKind = "Manual";
                recentCursor = -1;
            }

            const bool canSend =
                !busy && !input.empty() && !cfg.endpoint.empty() && !cfg.model.empty();
            ImGui::BeginDisabled(!canSend);
            if (ImGui::Button(busy ? "Sending..." : "Sift")) {
                const Config capturedCfg = cfg;
                const std::string capturedInput = input;
                const std::string capturedPrompt = prompt;
                const DiagnosticSplit split =
                    LooksLikeUnrealLog(input) &&
                    (cfg.profileId=="auto" || cfg.profileId=="unreal")
                        ? SplitUnrealDiagnostics(input, cfg)
                        : SplitWithProfile(input, cfg);
                questionableOutput = ApplyOutputPreferences(split.questionable, cfg);
                const std::string previewFiltered =
                    !split.included.empty() ? split.included : PreFilter(input, cfg);
                lastInputBytes = input.size();
                lastFilteredBytes = previewFiltered.size();
                stats.sourceKind = inputSourceKind;
                stats.logType = DetectLogType(input);
                stats.profile = ProfileName(input, cfg);
                stats.model = cfg.model;
                stats.compute = cfg.computeMode == 1 ? "GPU max" : cfg.computeMode == 2 ? "CPU" : "Auto";
                stats.inputBytes = input.size();
                stats.filteredBytes = previewFiltered.size();
                {
                    const auto [inputLineCount, inputWordCount] = HumanTextStats(input);
                    const auto [filteredLineCount, filteredWordCount] = HumanTextStats(previewFiltered);
                    stats.inputLines = inputLineCount;
                    stats.inputWords = inputWordCount;
                    stats.filteredLines = filteredLineCount;
                    stats.filteredWords = filteredWordCount;
                    stats.estimatedInputTokens = EstimateTokenCount(input);
                    stats.estimatedFilteredTokens = EstimateTokenCount(previewFiltered);
                }
                stats.route =
                    (cfg.preferFastPath && LooksLikeStructuredBuildDiagnostics(previewFiltered))
                        ? "Deterministic fast path"
                        : "LLM";
                stats.promptTokens = 0;
                stats.completionTokens = 0;
                stats.promptTokensPerSecond = 0.0;
                stats.completionTokensPerSecond = 0.0;
                stats.estimatedPromptTokensPerSecond = 0.0;
                requestStarted = std::chrono::steady_clock::now();
                lastResponseSeconds = 0.0;

                if (cfg.preferFastPath &&
                    LooksLikeStructuredBuildDiagnostics(previewFiltered)) {
                    output = ApplyOutputPreferences(
                        FastStructuredResult(previewFiltered), cfg);
                    lastResponseSeconds = std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - requestStarted).count();
                    stats.seconds = lastResponseSeconds;
                    const bool autoCopied =
                        MaybeAutoCopyResult(cfg, output, lastClipboardText);
                    if (autoCopied) markOwnClipboardWrite();
                    status = autoCopied
                        ? "Done - deterministic result auto-copied."
                        : "Done - deterministic fast path.";
                    AppendActivityLog(appLog, "SUCCESS",
                        "Manual deterministic sift completed with " +
                        std::to_string(DiagnosticEntries(output).size()) + " diagnostics.");
                    if (autoCopied)
                        AppendActivityLog(appLog, "AUTO-COPY", "Actionable manual result auto-copied.");
                    recordRecentRun();
                } else {
                    status = "Sending to model...";
                    AppendActivityLog(appLog, "LLM",
                        "Manual sift sent to model (" + std::to_string(input.size()) + " input bytes).");
                    ++requestGeneration;
                    activeRequestGeneration = requestGeneration;
                    busy = true;
                    activeProgress = std::make_shared<SiftProgress>();
                    const int initialChunks = static_cast<int>(ChunkModelInput(previewFiltered).size());
                    activeProgress->total = std::max(1, initialChunks);
                    activeProgress->chunking = initialChunks > 1;
                    if (initialChunks > 1) {
                        status = "Large log - chunking " + std::to_string(initialChunks) +
                            " model chunks; this may take longer.";
                        AppendActivityLog(appLog, "CHUNK",
                            "Manual log split into " + std::to_string(initialChunks) + " model chunks.");
                    }
                    request = LaunchSiftTask(
                        [capturedCfg, capturedInput, capturedPrompt, progress = activeProgress] {
                            return Send(capturedCfg, capturedInput, capturedPrompt, progress);
                        });
                }
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(!busy);
            if (ImGui::Button("Cancel##main_sift"))
                cancelActiveSift("main window");
            ImGui::EndDisabled();

            if (busy) {
                ImGui::SameLine();
                const double elapsed = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - requestStarted).count();
                ImGui::Text("Elapsed: %.2f s", elapsed);
            }

            if (lastInputBytes > 0) {
                const double retained = 100.0 *
                    static_cast<double>(lastFilteredBytes) /
                    static_cast<double>(lastInputBytes);
                ImGui::SameLine();
                const ImVec4 reductionColor = retained <= 25.0
                    ? ImVec4(0.30f, 0.90f, 0.48f, 1.0f)
                    : retained <= 60.0
                        ? ImVec4(0.35f, 0.75f, 1.0f, 1.0f)
                        : ImVec4(0.95f, 0.72f, 0.25f, 1.0f);
                ImGui::TextColored(reductionColor, "%.1f%% reduced",
                    100.0 - retained);
            }
            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("##results_pane", ImVec2(0, 0), ImGuiChildFlags_None,
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::SeparatorText("RESULTS");

            const auto [outputLines, outputWords] = HumanTextStats(output);
            const auto questionableEntries = DiagnosticEntries(questionableOutput);
            ImGui::TextDisabled("%zu lines  |  %zu words  |  %zu bytes",
                outputLines, outputWords, output.size());

            if (ImGui::BeginTabBar("##result_tabs")) {
                if (ImGui::BeginTabItem("Included")) {
                    const float resultActionReserve =
                        ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
                    const float resultHeight = std::max(
                        100.0f, ImGui::GetContentRegionAvail().y - resultActionReserve);
                    DrawDiagnosticEntries("##included_entries", output,
                        resultHeight, status, lastClipboardText, appLog, copyFlash);
                    ImGui::BeginDisabled(output.empty());
                    if (ImGui::Button("Copy Result")) {
                        if (SetOwnedClipboardText(output, &lastClipboardText)) {
                            StartCopyFlash(copyFlash, true);
                            AppendActivityLog(appLog, "COPY", "Result copied from main window.");
                        }
                    }
                    ImGui::EndDisabled();
                    ImGui::SameLine();
                    if (ImGui::Button("Clear")) {
                        input.clear();
                        output.clear();
                        questionableOutput.clear();
                        lastInputBytes = lastFilteredBytes = 0;
                        stats = {};
                        inputSourceKind = "Manual";
                        stats.sourceKind = inputSourceKind;
                        status = "Cleared.";
                    }
                    ImGui::EndTabItem();
                }

                const std::string questionableTab =
                    "Questionable / Excluded (" +
                    std::to_string(questionableEntries.size()) + ")";
                if (ImGui::BeginTabItem(questionableTab.c_str())) {
                    const auto [questionableLines, questionableWords] =
                        HumanTextStats(questionableOutput);
                    ImGui::TextDisabled(
                        "%zu lines  |  %zu words  |  %zu bytes  |  review only",
                        questionableLines, questionableWords,
                        questionableOutput.size());
                    const float questionableHeight =
                        std::max(100.0f, ImGui::GetContentRegionAvail().y);
                    DrawDiagnosticEntries("##questionable_entries",
                        questionableOutput, questionableHeight,
                        status, lastClipboardText, appLog, copyFlash);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndChild();
            ImGui::EndTable();
        }

            ImGui::EndTabItem();
            }

            const std::string recentsTabLabel =
                "Recents (" + std::to_string(recentRuns.size()) + ")###recents";
