// Popup preview/actions/footer/render completion.
// Included by main.cpp; keep this module focused on this responsibility.

                                    stats.completionTokensPerSecond);
                            } else if (stats.completionTokensPerSecond > 0.0) {
                                ImGui::Text("%.0f output tok/s",
                                    stats.completionTokensPerSecond);
                            } else {
                                ImGui::Text("%.0f prompt tok/s",
                                    stats.promptTokensPerSecond);
                            }
                        }
                        if (cfg.resultShowBytesReduction) {
                            const double reduced = stats.inputBytes > 0
                                ? 100.0 * (1.0 -
                                    static_cast<double>(stats.filteredBytes) /
                                    static_cast<double>(stats.inputBytes))
                                : 0.0;
                            const ImVec4 reductionColor = reduced >= 75.0
                                ? ImVec4(0.30f, 0.90f, 0.48f, 1.0f)
                                : reduced >= 40.0
                                    ? ImVec4(0.35f, 0.75f, 1.0f, 1.0f)
                                    : ImVec4(0.95f, 0.72f, 0.25f, 1.0f);
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Data");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%zu -> %zu bytes", stats.inputBytes, stats.filteredBytes);
                            ImGui::SameLine();
                            ImGui::TextColored(reductionColor, "%.1f%% reduced", reduced);
                        }
                        if (cfg.resultShowTime) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0); ImGui::TextDisabled("Time");
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%.2f s", stats.seconds);
                        }
                        ImGui::EndTable();
                    }

                    if (stats.ocr.present) {
                        ToastDisclosureRow(
                            "result_ocr_stats", "OCR pass", toastOcrStatsExpanded, outcomeColor);
                        if (toastOcrStatsExpanded &&
                            ImGui::BeginTable("##result_ocr_popup_stats", 2,
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

                    if (cfg.resultShowAutoCopy && toastAutoCopied) {
                        ImGui::Separator();
                        ImGui::TextColored(
                            ImVec4(0.32f, 0.92f, 0.58f, 1.0f),
                            "AUTO-COPIED TO CLIPBOARD");
                    }

                    if (cfg.resultShowCounts)
                        ImGui::TextDisabled("%zu included  |  %zu questionable",
                            entries, questionable);

                    if (cfg.resultShowPreview && !output.empty()) {
                        ImGui::Separator();
                        const auto previewEntries = DiagnosticEntries(output);
                        ToastDisclosureRow(
                            "result_preview",
                            ("Preview (" + std::to_string(previewEntries.size()) + ")").c_str(),
                            toastPreviewExpanded,
                            outcomeColor);

                        const size_t previewCount = std::min<size_t>(
                            previewEntries.size(),
                            static_cast<size_t>(std::clamp(cfg.toastPreviewLines, 1, 10)));
                        if (toastPreviewExpanded) {
                            int previewVisualLines = 0;
                            for (size_t i = 0; i < previewCount; ++i) {
                                const auto& previewEntry = previewEntries[i];
                                const int hardLines = 1 + static_cast<int>(
                                    std::count(previewEntry.begin(), previewEntry.end(), '\n'));
                                const int wrappedLines = std::max(
                                    hardLines,
                                    static_cast<int>((previewEntry.size() + 55) / 56));
                                previewVisualLines += wrappedLines;
                            }

                            const float previewHeight = static_cast<float>(
                                std::max(
                                    48,
                                    previewVisualLines * 18 +
                                        static_cast<int>(previewCount) * 8 + 10));
                            DrawToastPreviewEntries(
                                "##toast_preview_entries",
                                previewEntries,
                                previewCount,
                                previewHeight,
                                status,
                                lastClipboardText,
                                appLog,
                                copyFlash);

                            if (previewEntries.size() > previewCount)
                                ImGui::TextDisabled("+%zu more",
                                    previewEntries.size() - previewCount);
                        }
                    }
                }
                // Optional lifetime bar belongs to content, never below the
                // action row. Every popup therefore ends with the same footer.
                const bool showToastLifetimeBar =
                    toastOutcome == ToastOutcome::OcrPrompt
                        ? cfg.ocrPromptShowTimeoutBar
                        : cfg.resultShowLifetimeBar;
                if (!toastProcessing &&
                    toastOutcome != ToastOutcome::Cancelled &&
                    showToastLifetimeBar) {
                    ImGui::Spacing();
                    const auto nowToast = std::chrono::steady_clock::now();
                    const float remaining = toastTimerPaused
                        ? std::max(0.0f,
                            std::chrono::duration<float>(toastPausedRemaining).count())
                        : std::max(0.0f,
                            std::chrono::duration<float>(toastUntil - nowToast).count());
                    const float toastLifetimeSeconds =
                        toastOutcome == ToastOutcome::OcrPrompt
                            ? cfg.ocrPromptSeconds
                            : acknowledgementToast
                                ? 1.8f
                                : cfg.toastSeconds;
                    const float fraction = toastLifetimeSeconds > 0.0f
                        ? std::clamp(remaining / toastLifetimeSeconds, 0.0f, 1.0f)
                        : 0.0f;
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, outcomeColor);
                    ImGui::ProgressBar(fraction, {-1, 4}, "");
                    ImGui::PopStyleColor();
                }

                // Shared footer: every popup uses the exact same bottom inset.
                // Pinning this row means a slightly conservative body-height estimate
                // can never create a different-looking bottom between popup states.
                const float buttonH = 28.0f;
                const float openW = 82.0f, copyW = 112.0f, dismissW = 82.0f, cancelW = 82.0f, gap = 8.0f;
                const float footerY =
                    ImGui::GetWindowHeight() -
                    ImGui::GetStyle().WindowPadding.y -
                    buttonH;
                const float separatorY =
                    std::max(
                        ImGui::GetCursorPosY(),
                        footerY - ImGui::GetStyle().ItemSpacing.y - 2.0f);
                ImGui::SetCursorPosY(separatorY);
                ImGui::Separator();
                ImGui::SetCursorPosY(footerY);

                if (toastOutcome == ToastOutcome::OcrPrompt) {
                    const float startOcrW = 104.0f;
                    const float totalW = startOcrW + dismissW + gap;
                    ImGui::SetCursorPosX(std::max(
                        ImGui::GetStyle().WindowPadding.x,
                        ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - totalW));
                    ImGui::BeginDisabled(!pendingOcrImageReady);
                    if (ImGui::Button("Start OCR", {startOcrW, buttonH}) &&
                        pendingOcrImageReady) {
                        ClipboardImage image = std::move(pendingOcrImage);
                        pendingOcrImage = {};
                        pendingOcrImageReady = false;
                        beginOcrScan(std::move(image), "OCR", "Clipboard image");
                    }
                    ImGui::EndDisabled();
                    ImGui::SameLine(0.0f, gap);
                    if (ImGui::Button("Dismiss", {dismissW, buttonH})) {
                        pendingOcrImage = {};
                        pendingOcrImageReady = false;
                        toastText.clear();
                    }
                } else if (toastProcessing ||
                    toastOutcome == ToastOutcome::Cancelled ||
                    toastOutcome == ToastOutcome::Empty ||
                    acknowledgementToast) {
                    const bool finishedScanLayout =
                        toastOutcome == ToastOutcome::Cancelled ||
                        toastOutcome == ToastOutcome::Empty ||
                        acknowledgementToast;
                    const bool clipboardOcrInProgress =
                        toastProcessing && stats.sourceKind == "OCR";

                    if (clipboardOcrInProgress) {
                        ImGui::SetCursorPosX(std::max(
                            ImGui::GetStyle().WindowPadding.x,
                            ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - cancelW));
                        if (ImGui::Button("Cancel", {cancelW, buttonH}))
                            cancelActiveSift("notification");
                    } else {
                        const float secondW = finishedScanLayout ? dismissW : cancelW;
                        const float totalW = openW + secondW + gap;
                        ImGui::SetCursorPosX(std::max(
                            ImGui::GetStyle().WindowPadding.x,
                            ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - totalW));
                        if (ImGui::Button("Open", {openW, buttonH}))
                            reopenMainWindow();
                        ImGui::SameLine(0.0f, gap);
                        if (finishedScanLayout) {
                            if (ImGui::Button("Dismiss", {dismissW, buttonH}))
                                toastText.clear();
                        } else {
                            if (ImGui::Button("Cancel", {cancelW, buttonH}))
                                cancelActiveSift("notification");
                        }
                    }
                } else {
                    const float totalW = openW + copyW + dismissW + gap * 2.0f;
                    ImGui::SetCursorPosX(std::max(ImGui::GetStyle().WindowPadding.x,
                        ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - totalW));
                    if (ImGui::Button("Open", {openW, buttonH})) {
                        reopenMainWindow();
                        toastText.clear();
                    }
                    ImGui::SameLine(0.0f, gap);
                    ImGui::BeginDisabled(output.empty());
                    if (ImGui::Button("Copy Results", {copyW, buttonH})) {
                        if (SetOwnedClipboardText(output, &lastClipboardText)) {
                            StartCopyFlash(copyFlash, true);
#ifdef _WIN32
                            lastClipboardSequence = GetClipboardSequenceNumber();
#endif
                            status = "Result copied.";
                            AppendActivityLog(appLog, "COPY", "Result copied from notification.");
                        }
                    }
                    ImGui::EndDisabled();
                    ImGui::SameLine(0.0f, gap);
                    if (ImGui::Button("Dismiss", {dismissW, buttonH})) toastText.clear();
                }

                ImGui::End();
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor();
                ImGui::Render();
                SDL_SetRenderDrawColor(toastRenderer, 24,24,27,255);
                SDL_RenderClear(toastRenderer);
                ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), toastRenderer);
                SDL_RenderPresent(toastRenderer);

                ImGui::SetCurrentContext(mainContext);
            } else {
                SDL_HideWindow(toastWindow);
            }
        }
