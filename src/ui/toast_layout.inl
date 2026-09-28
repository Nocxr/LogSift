// Popup sizing, positioning, and frame setup.
// Included by main.cpp; keep this module focused on this responsibility.

                        !toastProcessing && toastOutcome == ToastOutcome::Processing;
                    const bool sizingOcrPrompt =
                        !toastProcessing && toastOutcome == ToastOutcome::OcrPrompt;
                    const bool sizingScanLayout =
                        toastProcessing ||
                        toastOutcome == ToastOutcome::Cancelled ||
                        toastOutcome == ToastOutcome::Empty ||
                        sizingAcknowledgement ||
                        sizingOcrPrompt;
                    const auto ocrPopupRows = [&]() {
                        if (!stats.ocr.present) return 0;
                        int rows = 1; // image input
                        if (stats.ocr.outputBytes > 0) ++rows;
                        if (stats.ocr.promptTokens > 0 || stats.ocr.completionTokens > 0) ++rows;
                        if (stats.ocr.promptTokensPerSecond > 0.0 ||
                            stats.ocr.completionTokensPerSecond > 0.0) ++rows;
                        if (stats.ocr.seconds > 0.0) ++rows;
                        return rows;
                    };

                    int contentHeight = 78; // shared popup frame; content adds the rest
                    if (sizingOcrPrompt) {
                        // OCR confirmation has a fixed, known structure:
                        // header + two prompt lines + image disclosure + action row.
                        // Do not squeeze it through the generic scan estimate; that
                        // was what allowed the buttons to overlap the bottom edge.
                        contentHeight = 178;
                        if (cfg.ocrPromptShowTimeoutBar)
                            contentHeight += 14;
                        if (cfg.ocrPromptShowImageDetails && stats.ocr.present) {
                            contentHeight += 28; // Image disclosure row.
                            if (toastOcrStatsExpanded)
                                contentHeight += ocrPopupRows() * 22;
                        }
                    } else if (sizingScanLayout) {
                        if (cfg.scanShowProgress)
                            contentHeight += sizingChunking ? 54 : 34;

                        const bool hasScanStats =
                            !sizingAcknowledgement &&
                            (cfg.scanShowSource || cfg.scanShowModel || cfg.scanShowRoute ||
                             cfg.scanShowPrefilterCounts || cfg.scanShowEstimatedTokens ||
                             cfg.scanShowBytesReduction || cfg.scanShowElapsedTime);
                        if (hasScanStats) {
                            contentHeight += 28; // Stats disclosure row
                            if (toastStatsExpanded) {
                                if (cfg.scanShowSource) contentHeight += 22;
                                if (cfg.scanShowModel) contentHeight += 22;
                                if (cfg.scanShowRoute) contentHeight += 22;
                                if (cfg.scanShowPrefilterCounts) contentHeight += 22;
                                if (cfg.scanShowEstimatedTokens) contentHeight += 22;
                                if (cfg.scanShowBytesReduction) contentHeight += 22;
                                if (cfg.scanShowElapsedTime) contentHeight += 22;
                            }
                        }
                        if (stats.ocr.present) {
                            contentHeight += 28;
                            if (toastOcrStatsExpanded)
                                contentHeight += ocrPopupRows() * 22;
                        }
                        if (!toastProcessing &&
                            toastOutcome != ToastOutcome::Cancelled &&
                            cfg.resultShowLifetimeBar)
                            contentHeight += 14;
                    } else {
                        if (cfg.resultShowDiagnosticTotal) contentHeight += 22;
                        if (cfg.resultShowFallbackNotice &&
                            (toastOutcome == ToastOutcome::OfflineFallback ||
                             toastOutcome == ToastOutcome::ModelFallback))
                            contentHeight += 58;

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
                            contentHeight += 28; // Stats disclosure row
                            if (toastStatsExpanded) {
                                if (cfg.resultShowSource) contentHeight += 22;
                                if (cfg.resultShowModel) contentHeight += 22;
                                if (cfg.resultShowRoute) contentHeight += 22;
                                if (cfg.resultShowPrefilterCounts) contentHeight += 22;
                                if (cfg.resultShowEstimatedTokens) contentHeight += 22;
                                if (cfg.resultShowRealTokens &&
                                    (stats.promptTokens > 0 || stats.completionTokens > 0))
                                    contentHeight += 22;
                                if (cfg.resultShowTokenSpeed &&
                                    (stats.promptTokensPerSecond > 0.0 ||
                                     stats.completionTokensPerSecond > 0.0))
                                    contentHeight += 22;
                                if (cfg.resultShowBytesReduction) contentHeight += 22;
                                if (cfg.resultShowTime) contentHeight += 22;
                            }
                        }
                        if (stats.ocr.present) {
                            contentHeight += 28;
                            if (toastOcrStatsExpanded)
                                contentHeight += ocrPopupRows() * 22;
                        }

                        if (cfg.resultShowAutoCopy && toastAutoCopied) contentHeight += 34;
                        if (cfg.resultShowCounts) contentHeight += 22;

                        if (cfg.resultShowPreview && !output.empty()) {
                            contentHeight += 30; // Preview disclosure row
                            if (toastPreviewExpanded) {
                                const auto sizingPreviewEntries = DiagnosticEntries(output);
                                const int previewRows = static_cast<int>(std::min<size_t>(
                                    sizingPreviewEntries.size(),
                                    static_cast<size_t>(std::clamp(cfg.toastPreviewLines, 1, 10))));
                                int previewVisualLines = 0;
                                for (int i = 0; i < previewRows; ++i) {
                                    const auto& previewEntry = sizingPreviewEntries[i];
                                    const int hardLines = 1 + static_cast<int>(
                                        std::count(previewEntry.begin(), previewEntry.end(), '\n'));
                                    const int wrappedLines = std::max(
                                        hardLines,
                                        static_cast<int>((previewEntry.size() + 55) / 56));
                                    previewVisualLines += wrappedLines;
                                }
                                contentHeight += std::max(
                                    48, previewVisualLines * 18 + previewRows * 8 + 10);
                                if (sizingPreviewEntries.size() > static_cast<size_t>(previewRows))
                                    contentHeight += 20;
                            }
                        }

                        if (cfg.resultShowLifetimeBar &&
                            toastOutcome != ToastOutcome::Cancelled)
                            contentHeight += 14;
                    }

                    contentHeight += 8; // shared footer breathing room
                    const int maxToastHeight = std::max(220, usable.h - 36);
                    const int th = std::clamp(contentHeight, 180, maxToastHeight);
                    int currentW = 0, currentH = 0;
                    SDL_GetWindowSize(toastWindow, &currentW, &currentH);
                    if (currentW != tw || currentH != th)
                        SDL_SetWindowSize(toastWindow, tw, th);
                    SDL_SetWindowPosition(
                        toastWindow,
                        usable.x + usable.w - tw - 18,
                        usable.y + usable.h - th - 18);
                }
                if (displays) SDL_free(displays);

                if (toastWasHidden) {
                    SDL_ShowWindow(toastWindow);
                    SDL_RaiseWindow(toastWindow);
                }

                ImGui::SetCurrentContext(toastContext);
                ImGui_ImplSDLRenderer3_NewFrame();
                ImGui_ImplSDL3_NewFrame();
                ImGui::NewFrame();
                ImGui::SetNextWindowPos({0,0});
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                ImVec4 outcomeColor(cfg.toastAccent[0], cfg.toastAccent[1], cfg.toastAccent[2], 1.0f);
                if (toastOutcome == ToastOutcome::Success) outcomeColor = ImVec4(0.22f, 0.78f, 0.40f, 1.0f);
                else if (toastOutcome == ToastOutcome::OcrPrompt) outcomeColor = ImVec4(0.28f, 0.82f, 1.00f, 1.0f);
