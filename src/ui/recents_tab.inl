// Recents tab rendering and history interactions.
// Included by main.cpp; keep this module focused on this responsibility.

            if (ImGui::BeginTabItem(recentsTabLabel.c_str())) {
                if (recentRuns.empty()) {
                    ImGui::TextDisabled(
                        "No recent runs yet. Completed sifts will appear here.");
                } else {
                    selectedRecent = std::clamp(
                        selectedRecent, 0, static_cast<int>(recentRuns.size()) - 1);

                    if (ImGui::BeginTable("##recent_runs_layout", 2,
                        ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV |
                        ImGuiTableFlags_SizingStretchProp,
                        ImVec2(-1.0f, ImGui::GetContentRegionAvail().y))) {

                        ImGui::TableSetupColumn(
                            "History", ImGuiTableColumnFlags_WidthFixed, 285.0f);
                        ImGui::TableSetupColumn(
                            "Details", ImGuiTableColumnFlags_WidthStretch);
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::BeginChild(
                            "##recent_list", {0, 0}, ImGuiChildFlags_None);
                        ImGui::SeparatorText("RECENT RUNS");
                        for (size_t i = 0; i < recentRuns.size(); ++i) {
                            const RecentRun& run = recentRuns[i];
                            ImGui::PushID(static_cast<int>(i));
                            const bool selected =
                                static_cast<int>(i) == selectedRecent;
                            const std::string label =
                                run.timestamp + "  " +
                                (run.sourceKind.empty() ? "Manual" : run.sourceKind);
                            if (ImGui::Selectable(label.c_str(), selected)) {
                                selectedRecent = static_cast<int>(i);
                            }
                            ImGui::TextDisabled("%s  |  %s",
                                run.logType.c_str(), run.route.c_str());
                            std::string inputPreview = run.input;
                            std::replace(
                                inputPreview.begin(), inputPreview.end(), '\n', ' ');
                            std::replace(
                                inputPreview.begin(), inputPreview.end(), '\r', ' ');
                            if (inputPreview.size() > 72)
                                inputPreview = inputPreview.substr(0, 69) + "...";
                            if (!inputPreview.empty())
                                ImGui::TextDisabled("%s", inputPreview.c_str());
                            const size_t diagCount =
                                DiagnosticEntries(run.output).size();
                            ImGui::TextDisabled(
                                "%zu diagnostic%s  |  %.2f s",
                                diagCount, diagCount == 1 ? "" : "s",
                                run.seconds);
                            if (i + 1 < recentRuns.size())
                                ImGui::Separator();
                            ImGui::PopID();
                        }
                        ImGui::EndChild();

                        ImGui::TableSetColumnIndex(1);
                        ImGui::BeginChild(
                            "##recent_detail", {0, 0}, ImGuiChildFlags_None);
                        RecentRun& run =
                            recentRuns[static_cast<size_t>(selectedRecent)];

                        ImGui::SeparatorText("RECENT RESULT");
                        ImGui::TextColored(
                            ImVec4(0.42f, 0.78f, 1.00f, 1.0f),
                            "%s", run.timestamp.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled(
                            "%s  |  %s  |  %s",
                            run.sourceKind.c_str(),
                            run.logType.c_str(),
                            run.profile.c_str());

                        ImGui::TextColored(
                            ImVec4(0.38f, 0.88f, 0.56f, 1.0f),
                            "%s", run.route.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled(
                            "%s  |  %.2f s  |  %d + %d tok",
                            run.model.c_str(),
                            run.seconds,
                            run.promptTokens,
                            run.completionTokens);
                        if (run.ocr.present) {
                            ImGui::TextColored(
                                ImVec4(0.25f, 0.88f, 0.78f, 1.0f),
                                "OCR: %.2f s  |  %d + %d tok  |  %zu bytes extracted",
                                run.ocr.seconds,
                                run.ocr.promptTokens,
                                run.ocr.completionTokens,
                                run.ocr.outputBytes);
                        }

                        if (ImGui::Button("Load into Sift")) {
                            applyRecentRun(selectedRecent);
                        }
                        ImGui::SameLine();
                        ImGui::BeginDisabled(run.output.empty());
                        if (ImGui::Button("Copy Result")) {
                            if (SetOwnedClipboardText(
                                    run.output, &lastClipboardText)) {
                                StartCopyFlash(copyFlash, true);
                                status = "Recent result copied.";
                                markOwnClipboardWrite();
                            }
                        }
                        ImGui::EndDisabled();
                        ImGui::SameLine();
                        if (ImGui::Button("Delete")) {
                            recentRuns.erase(
                                recentRuns.begin() + selectedRecent);
                            if (recentRuns.empty()) {
                                selectedRecent = -1;
                                recentCursor = -1;
                            } else {
                                selectedRecent = std::min(
                                    selectedRecent,
                                    static_cast<int>(recentRuns.size()) - 1);
                                recentCursor = std::min(
                                    std::max(0, recentCursor),
                                    static_cast<int>(recentRuns.size()) - 1);
                            }
                            SaveRecentRuns(recentRuns);
                        }

                        if (!recentRuns.empty() && selectedRecent >= 0) {
                            RecentRun& visibleRun =
                                recentRuns[static_cast<size_t>(selectedRecent)];
                            if (ImGui::BeginTabBar("##recent_content_tabs")) {
                                if (ImGui::BeginTabItem("Input")) {
                                    ImGui::InputTextMultiline(
                                        "##recent_input",
                                        &visibleRun.input,
                                        {-1, -1},
                                        ImGuiInputTextFlags_ReadOnly);
                                    ImGui::EndTabItem();
                                }
                                if (ImGui::BeginTabItem("Output")) {
                                    const float h =
                                        std::max(
                                            120.0f,
                                            ImGui::GetContentRegionAvail().y);
                                    DrawDiagnosticEntries(
                                        "##recent_output",
                                        visibleRun.output,
                                        h,
                                        status,
                                        lastClipboardText,
                                        appLog,
                                        copyFlash);
                                    ImGui::EndTabItem();
                                }
                                const size_t questionableCount =
                                    DiagnosticEntries(
                                        visibleRun.questionable).size();
                                const std::string qLabel =
                                    "Questionable (" +
                                    std::to_string(questionableCount) + ")";
                                if (ImGui::BeginTabItem(qLabel.c_str())) {
                                    const float h =
                                        std::max(
                                            120.0f,
                                            ImGui::GetContentRegionAvail().y);
                                    DrawDiagnosticEntries(
                                        "##recent_questionable",
                                        visibleRun.questionable,
                                        h,
                                        status,
                                        lastClipboardText,
                                        appLog,
                                        copyFlash);
                                    ImGui::EndTabItem();
                                }
                                ImGui::EndTabBar();
                            }
                        }
                        ImGui::EndChild();
                        ImGui::EndTable();
                    }
                }
                ImGui::EndTabItem();
            }

