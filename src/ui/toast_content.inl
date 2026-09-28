// Popup state-specific content and stats rendering.
// Included by main.cpp; keep this module focused on this responsibility.

                else if (toastOutcome == ToastOutcome::Empty) outcomeColor = ImVec4(0.88f, 0.68f, 0.20f, 1.0f);
                else if (toastOutcome == ToastOutcome::OfflineFallback) outcomeColor = ImVec4(0.20f, 0.82f, 1.00f, 1.0f);
                else if (toastOutcome == ToastOutcome::ModelFallback) outcomeColor = ImVec4(0.78f, 0.48f, 1.00f, 1.0f);
                else if (toastOutcome == ToastOutcome::Cancelled) outcomeColor = ImVec4(0.72f, 0.74f, 0.78f, 1.0f);
                else if (toastOutcome == ToastOutcome::Failure) outcomeColor = ImVec4(0.92f, 0.28f, 0.28f, 1.0f);
                const ImVec4 toastBackground =
                    toastOutcome == ToastOutcome::OfflineFallback
                        ? ImVec4(0.035f, 0.12f, 0.18f, 1.0f)
                        : toastOutcome == ToastOutcome::ModelFallback
                            ? ImVec4(0.11f, 0.055f, 0.16f, 1.0f)
                            : ImVec4(cfg.toastBg[0], cfg.toastBg[1], cfg.toastBg[2], 1.0f);
                ImGui::PushStyleColor(ImGuiCol_WindowBg, toastBackground);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 13));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 7));
                ImGui::Begin("##toast_root", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoSavedSettings);
                const size_t entries = DiagnosticEntries(output).size();
                const size_t questionable = DiagnosticEntries(questionableOutput).size();
                const bool chunkingNow =
                    toastProcessing && activeProgress && activeProgress->chunking.load();
                if (chunkingNow)
                    outcomeColor = ImVec4(0.96f, 0.60f, 0.20f, 1.0f);
                const bool acknowledgementToast =
                    !toastProcessing && toastOutcome == ToastOutcome::Processing;
                const char* outcomeLabel = toastProcessing
                    ? (chunkingNow ? "LOG SIFT - CHUNKING LARGE LOG" : "LOG SIFT - SCANNING")
                    : acknowledgementToast ? "LOG SIFT - CLIPBOARD DETECTED" :
                    toastOutcome == ToastOutcome::OcrPrompt ? "LOG SIFT - IMAGE DETECTED" :
                    toastOutcome == ToastOutcome::Success ? "LOG SIFT - COMPLETE" :
                    toastOutcome == ToastOutcome::Empty ? "LOG SIFT - NOTHING FOUND" :
                    toastOutcome == ToastOutcome::OfflineFallback ? "LOG SIFT - OFFLINE FALLBACK" :
                    toastOutcome == ToastOutcome::ModelFallback ? "LOG SIFT - MODEL RESPONSE FALLBACK" :
                    toastOutcome == ToastOutcome::Cancelled ? "LOG SIFT - CANCELLED" :
                    toastOutcome == ToastOutcome::Failure ? "LOG SIFT - FAILED" : "LOG SIFT";
                const float titleY = ImGui::GetCursorPosY();
                ImGui::TextColored(outcomeColor, "%s", outcomeLabel);
                const float titleRightX =
                    ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x;

                const float closeSize = 26.0f;
                const float topButtonGap = 6.0f;
                const float controlGroupWidth =
                    closeSize * 3.0f + topButtonGap * 2.0f;
                const float controlsRight =
                    ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x;
                const float topControlsX =
                    controlsRight - controlGroupWidth;
                const float topRowHeight = std::max(26.0f, ImGui::GetTextLineHeight());
                const float topControlsY =
                    titleY + (topRowHeight - closeSize) * 0.5f;

                const ImVec2 badgeSize = ToastSourceBadgeSize(stats.sourceKind);
                const float badgeAreaLeft = titleRightX + 8.0f;
                const float badgeAreaRight = topControlsX - 8.0f;
                const float badgeAreaWidth =
                    std::max(0.0f, badgeAreaRight - badgeAreaLeft);
                const float badgeX =
                    badgeAreaLeft + std::max(
                        0.0f,
                        (badgeAreaWidth - badgeSize.x) * 0.5f);
                const float badgeY =
                    titleY + (topRowHeight - badgeSize.y) * 0.5f;
                ImGui::SetCursorPos(ImVec2(badgeX, badgeY));
                DrawToastSourceBadge(stats.sourceKind, outcomeColor);

                ImGui::SetCursorPos(ImVec2(topControlsX, topControlsY));
                if (ToastGearIconButton(
                        ImVec4(0.78f, 0.84f, 0.90f, 1.0f), closeSize)) {
                    SDL_ShowWindow(window);
                    SDL_RaiseWindow(window);
                    status = "Opened Log Sift from notification.";
                    AppendActivityLog(appLog, "UI",
                        "Main window opened from notification gear.");
                }

                ImGui::SetCursorPos(ImVec2(
                    topControlsX + closeSize + topButtonGap,
                    topControlsY));
                const ImVec4 soundIconColor = cfg.toastSound
                    ? outcomeColor
                    : ImVec4(0.58f, 0.60f, 0.64f, 1.0f);
                if (ToastSoundIconButton(cfg.toastSound, soundIconColor, closeSize)) {
                    cfg.toastSound = !cfg.toastSound;
#ifdef _WIN32
                    gTraySoundEnabled = cfg.toastSound;
#elif defined(__APPLE__)
                    LogSiftMacTraySetSound(cfg.toastSound);
#endif
                    status = cfg.toastSound
                        ? "Notification sound enabled."
                        : "Notification sound disabled.";
                    SaveConfig(cfg);
                    lastSavedConfig = ConfigToJson(cfg).dump();
                    AppendActivityLog(appLog, "SOUND", status);
                    if (!toastProcessing)
                        toastSoundPlayed = true;
                }

                ImGui::SetCursorPos(ImVec2(
                    topControlsX + closeSize * 2.0f + topButtonGap * 2.0f,
                    topControlsY));
                if (ToastCloseIconButton(ImVec4(0.88f, 0.90f, 0.93f, 1.0f), closeSize)) {
                    AppendActivityLog(appLog, "UI",
                        toastProcessing
                            ? "Scanning notification hidden; sift continues."
                            : "Notification closed.");
                    if (toastOutcome == ToastOutcome::OcrPrompt) {
                        pendingOcrImage = {};
                        pendingOcrImageReady = false;
                    }
                    toastText.clear();
                }
                ImGui::SetCursorPosY(std::max(
                    ImGui::GetCursorPosY(),
                    titleY + std::max(closeSize, 26.0f) + 2.0f));

                const bool scanLayout =
                    toastProcessing ||
                    toastOutcome == ToastOutcome::Cancelled ||
                    toastOutcome == ToastOutcome::Empty ||
                    toastOutcome == ToastOutcome::OcrPrompt ||
                    acknowledgementToast;
                if (scanLayout) {
                    const double elapsed = toastProcessing
                        ? std::chrono::duration<double>(
                            std::chrono::steady_clock::now() - requestStarted).count()
                        : stats.seconds;

                    if (cfg.scanShowProgress) {
                        if (toastOutcome == ToastOutcome::Cancelled) {
                            ImGui::TextColored(outcomeColor, "Sift cancelled");
                            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, outcomeColor);
                            ImGui::ProgressBar(1.0f, {-1, 5}, "");
                            ImGui::PopStyleColor();
                        } else if (toastOutcome == ToastOutcome::OcrPrompt) {
                            ImGui::TextColored(outcomeColor, "Image ready for OCR");
                            ImGui::TextDisabled("Start OCR when you want to send this image to the model.");
                        } else if (toastOutcome == ToastOutcome::Empty) {
                            ImGui::TextColored(outcomeColor, "No actionable diagnostics found");
                            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, outcomeColor);
                            ImGui::ProgressBar(1.0f, {-1, 5}, "");
                            ImGui::PopStyleColor();
                        } else if (acknowledgementToast) {
                            ImGui::TextColored(outcomeColor, "Clipboard change detected");
                            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, outcomeColor);
                            ImGui::ProgressBar(1.0f, {-1, 5}, "");
                            ImGui::PopStyleColor();
                        } else if (chunkingNow) {
                            const int done = activeProgress->completed.load();
                            const int total = std::max(1, activeProgress->total.load());
                            ImGui::TextColored(outcomeColor, "Chunk %d / %d",
                                std::min(done + 1, total), total);
                            ImGui::SameLine();
                            ImGui::TextDisabled("Large log - chunking may take longer");
                            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, outcomeColor);
                            ImGui::ProgressBar(
                                std::clamp(static_cast<float>(done) / static_cast<float>(total), 0.0f, 1.0f),
                                {-1, 5}, "");
                            ImGui::PopStyleColor();
                        } else {
                            ImGui::TextColored(outcomeColor, "Model sift in progress");
                            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, outcomeColor);
                            ImGui::ProgressBar(
                                -1.0f * static_cast<float>(ImGui::GetTime()), {-1, 5}, "");
                            ImGui::PopStyleColor();
                        }
                    }

                    if (!acknowledgementToast &&
                        toastOutcome != ToastOutcome::OcrPrompt &&
                        (cfg.scanShowSource || cfg.scanShowModel || cfg.scanShowRoute ||
                         cfg.scanShowPrefilterCounts || cfg.scanShowEstimatedTokens ||
                         cfg.scanShowBytesReduction || cfg.scanShowElapsedTime)) {
                        ToastDisclosureRow("scan_stats", "Stats", toastStatsExpanded, outcomeColor);
                        if (toastStatsExpanded && ImGui::BeginTable("##scan_popup_stats", 2,
                            ImGuiTableFlags_SizingStretchProp)) {
                            ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, 102.0f);
                            ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

                            if (cfg.scanShowSource) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Source");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted(stats.logType.c_str());
                            }
                            if (cfg.scanShowModel) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Model");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::TextUnformatted(stats.model.empty() ? cfg.model.c_str() : stats.model.c_str());
                            }
                            if (cfg.scanShowRoute) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Route");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%s  |  %s",
                                    stats.route.c_str(),
                                    stats.compute.empty() ? "Auto" : stats.compute.c_str());
                            }
                            if (cfg.scanShowPrefilterCounts) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Prefilter");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%zu -> %zu lines  |  %zu -> %zu words",
                                    stats.inputLines, stats.filteredLines,
                                    stats.inputWords, stats.filteredWords);
                            }
                            if (cfg.scanShowEstimatedTokens) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Est. tokens");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("~%zu -> ~%zu",
                                    stats.estimatedInputTokens, stats.estimatedFilteredTokens);
                            }
                            if (cfg.scanShowBytesReduction) {
                                const double scanReduced = stats.inputBytes > 0
                                    ? 100.0 * (1.0 -
                                        static_cast<double>(stats.filteredBytes) /
                                        static_cast<double>(stats.inputBytes))
                                    : 0.0;
                                const ImVec4 scanReductionColor = scanReduced >= 75.0
                                    ? ImVec4(0.30f, 0.90f, 0.48f, 1.0f)
                                    : scanReduced >= 40.0
                                        ? ImVec4(0.35f, 0.75f, 1.0f, 1.0f)
                                        : ImVec4(0.95f, 0.72f, 0.25f, 1.0f);
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Data");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%zu -> %zu bytes", stats.inputBytes, stats.filteredBytes);
                                ImGui::SameLine();
                                ImGui::TextColored(scanReductionColor, "%.1f%% reduced", scanReduced);
                            }
                            if (cfg.scanShowElapsedTime) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Elapsed");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%.1f s", elapsed);
                            }
                            ImGui::EndTable();
                        }
                    }

                    if (stats.ocr.present &&
                        (toastOutcome != ToastOutcome::OcrPrompt ||
                         cfg.ocrPromptShowImageDetails)) {
                        ToastDisclosureRow(
                            "scan_ocr_stats",
                            toastOutcome == ToastOutcome::OcrPrompt ? "Image" : "OCR pass",
                            toastOcrStatsExpanded,
                            outcomeColor);
                        if (toastOcrStatsExpanded &&
                            ImGui::BeginTable("##scan_ocr_popup_stats", 2,
                                ImGuiTableFlags_SizingStretchProp)) {
                            ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, 102.0f);
                            ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Image");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s  |  %zu bytes",
                                stats.ocr.mimeType.empty() ? "image" : stats.ocr.mimeType.c_str(),
                                stats.ocr.imageBytes);

                            if (stats.ocr.outputBytes > 0) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Extracted");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%zu bytes  |  %zu lines  |  %zu words",
                                    stats.ocr.outputBytes, stats.ocr.outputLines,
                                    stats.ocr.outputWords);
                            }
                            if (stats.ocr.promptTokens > 0 || stats.ocr.completionTokens > 0) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("OCR tokens");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%d prompt + %d output",
                                    stats.ocr.promptTokens, stats.ocr.completionTokens);
                            }
                            if (stats.ocr.promptTokensPerSecond > 0.0 ||
                                stats.ocr.completionTokensPerSecond > 0.0) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("OCR speed");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%.0f prompt/s  |  %.0f output/s",
                                    stats.ocr.promptTokensPerSecond,
                                    stats.ocr.completionTokensPerSecond);
                            }
                            if (stats.ocr.seconds > 0.0) {
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("OCR time");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%.2f s", stats.ocr.seconds);
                            }
                            ImGui::EndTable();
                        }
                    }
                } else {
                    if (cfg.resultShowDiagnosticTotal)
                        ImGui::TextColored(outcomeColor, "%zu diagnostic%s",
                            entries, entries == 1 ? "" : "s");

                    if (cfg.resultShowFallbackNotice &&
                        toastOutcome == ToastOutcome::OfflineFallback) {
                        ImGui::Separator();
                        ImGui::TextColored(outcomeColor,
                            "MODEL ENDPOINT OFFLINE - LOCAL FILTER ONLY");
                        ImGui::TextWrapped(
                            "Showing conservative local results; more candidates may be included.");
                    } else if (cfg.resultShowFallbackNotice &&
                        toastOutcome == ToastOutcome::ModelFallback) {
                        ImGui::Separator();
                        ImGui::TextColored(outcomeColor,
                            "MODEL ONLINE - LOCAL FILTER USED");
                        ImGui::TextWrapped(
                            "Connectivity is OK. The model response was not usable enough, so Log Sift kept conservative local diagnostics.");
                    }

                    std::string sourceLabel = stats.logType;
                    const bool redundantProfile =
                        (stats.logType == "Unreal" && stats.profile == "Unreal Engine") ||
                        stats.profile.empty() || stats.profile == stats.logType;
                    if (!redundantProfile && stats.profile != "Generic")
                        sourceLabel += " / " + stats.profile;

                    const bool hasResultStats =
                        cfg.resultShowSource || cfg.resultShowModel || cfg.resultShowRoute ||
                        cfg.resultShowPrefilterCounts || cfg.resultShowEstimatedTokens ||
                        (cfg.resultShowRealTokens &&
                            (stats.promptTokens > 0 || stats.completionTokens > 0)) ||
                        (cfg.resultShowTokenSpeed &&
                            (stats.promptTokensPerSecond > 0.0 ||
                             stats.completionTokensPerSecond > 0.0)) ||
                        cfg.resultShowBytesReduction || cfg.resultShowTime;

                    if (hasResultStats) {
                        ToastDisclosureRow("result_stats", "Stats", toastStatsExpanded, outcomeColor);
                    }
                    if (hasResultStats && toastStatsExpanded &&
                        ImGui::BeginTable("##result_popup_stats", 2,
                            ImGuiTableFlags_SizingStretchProp)) {
                        ImGui::TableSetupColumn("##label", ImGuiTableColumnFlags_WidthFixed, 102.0f);
                        ImGui::TableSetupColumn("##value", ImGuiTableColumnFlags_WidthStretch);

                        if (cfg.resultShowSource) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Source");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextUnformatted(sourceLabel.c_str());
                        }
                        if (cfg.resultShowModel) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Model");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextUnformatted(stats.model.empty() ? cfg.model.c_str() : stats.model.c_str());
                        }
                        if (cfg.resultShowRoute) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Route");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s  |  %s",
                                stats.route.c_str(),
                                stats.compute.empty() ? "Auto" : stats.compute.c_str());
                        }
                        if (cfg.resultShowPrefilterCounts) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Prefilter");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%zu -> %zu lines  |  %zu -> %zu words",
                                stats.inputLines, stats.filteredLines,
                                stats.inputWords, stats.filteredWords);
                        }
                        if (cfg.resultShowEstimatedTokens) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Est. tokens");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("~%zu -> ~%zu",
                                stats.estimatedInputTokens, stats.estimatedFilteredTokens);
                        }
                        if (cfg.resultShowRealTokens &&
                            (stats.promptTokens > 0 || stats.completionTokens > 0)) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("LLM tokens");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextColored(ImVec4(0.42f, 0.78f, 1.00f, 1.0f),
                                "%d prompt + %d output",
                                stats.promptTokens, stats.completionTokens);
                            ImGui::SameLine();
                            ImGui::TextDisabled("(real)");
                        }
                        if (cfg.resultShowTokenSpeed &&
                            (stats.promptTokensPerSecond > 0.0 ||
                             stats.completionTokensPerSecond > 0.0)) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Token speed");
                            ImGui::TableSetColumnIndex(1);
                            if (stats.promptTokensPerSecond > 0.0 &&
                                stats.completionTokensPerSecond > 0.0) {
                                ImGui::Text("%.0f prompt/s  |  %.0f output/s",
                                    stats.promptTokensPerSecond,
