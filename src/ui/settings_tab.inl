// Settings tab rendering.
// Included by main.cpp; keep this module focused on this responsibility.

            if (ImGui::BeginTabItem("Settings")) {
                ImGui::BeginChild("##settings_scroller", ImVec2(0, 0), ImGuiChildFlags_None);
                ImGui::SeparatorText("GENERAL");
                if (ImGui::Checkbox("Start Log Sift at login", &startAtLogin)) {
                    bool applied = false;
#ifdef _WIN32
                    applied = WindowsSetStartAtLogin(startAtLogin);
#elif defined(__APPLE__)
                    applied = LogSiftMacSetStartAtLogin(startAtLogin);
#endif
                    if (!applied) {
                        startAtLogin = !startAtLogin;
                        status = "Could not update start-at-login setting.";
                    } else {
                        status = startAtLogin ? "Log Sift will start at login." : "Start at login disabled.";
                        AppendActivityLog(appLog, "SETTINGS", status);
                    }
                }
                if (ImGui::Checkbox("Auto-copy actionable results", &cfg.autoCopyResults)) {
#ifdef _WIN32
                    gTrayAutoCopyEnabled = cfg.autoCopyResults;
#elif defined(__APPLE__)
                    LogSiftMacTraySetAutoCopy(cfg.autoCopyResults);
#endif
                    AppendActivityLog(appLog, "AUTO-COPY",
                        cfg.autoCopyResults ? "Auto copy enabled from Settings." : "Auto copy disabled from Settings.");
                }
                ImGui::SameLine();
                ImGui::TextDisabled("Copies only when Log Sift found diagnostics; never copies NO_DIAGNOSTICS/empty results.");

                ImGui::SetNextItemWidth(120);
                if (ImGui::SliderInt(
                        "Recent runs", &cfg.recentLimit, 1, 20, "%d")) {
                    if (static_cast<int>(recentRuns.size()) > cfg.recentLimit) {
                        recentRuns.resize(
                            static_cast<size_t>(cfg.recentLimit));
                        SaveRecentRuns(recentRuns);
                    }
                    if (recentRuns.empty()) {
                        recentCursor = -1;
                        selectedRecent = -1;
                    } else {
                        recentCursor = std::clamp(
                            recentCursor < 0 ? 0 : recentCursor,
                            0,
                            static_cast<int>(recentRuns.size()) - 1);
                        selectedRecent = std::clamp(
                            selectedRecent < 0 ? 0 : selectedRecent,
                            0,
                            static_cast<int>(recentRuns.size()) - 1);
                    }
                }
                ImGui::SameLine();
                ImGui::TextDisabled("Persistent input/output history.");
                ImGui::SameLine();
                ImGui::BeginDisabled(recentRuns.empty());
                if (ImGui::Button("Clear Recents")) {
                    recentRuns.clear();
                    recentCursor = -1;
                    selectedRecent = -1;
                    SaveRecentRuns(recentRuns);
                    status = "Recent run history cleared.";
                }
                ImGui::EndDisabled();

                ImGui::TextDisabled("Settings: %s", SettingsPath().string().c_str());
                ImGui::SameLine();
                if (ImGui::Button("Open Data Folder")) {
#ifdef _WIN32
                    ShellExecuteW(nullptr, L"open", UserDataDir().wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
                    SDL_OpenURL(("file://" + UserDataDir().string()).c_str());
#endif
                }

                ImGui::SeparatorText("PROFILES");
                ImGui::Text("Loaded: %zu", gProfiles.size());
                ImGui::SameLine();
                if (ImGui::Button("Reload Profiles")) LoadProfiles(argc > 0 ? argv[0] : nullptr);
                ImGui::SameLine();
                if (ImGui::Button("Open Profiles Folder") && !gProfilesDir.empty()) {
#ifdef _WIN32
                    ShellExecuteW(nullptr, L"open", gProfilesDir.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
                    SDL_OpenURL(("file://" + gProfilesDir.string()).c_str());
#endif
                }
                ImGui::TextDisabled("%s", gProfilesDir.empty() ? "No profile directory found." : gProfilesDir.string().c_str());
                ImGui::SeparatorText("NOTIFICATION");
                ImGui::TextUnformatted("Appearance");
                ImGui::ColorEdit3("Background", cfg.toastBg, ImGuiColorEditFlags_NoInputs);
                ImGui::SameLine();
                ImGui::ColorEdit3("Accent", cfg.toastAccent, ImGuiColorEditFlags_NoInputs);
                ImGui::SliderFloat("Visible time", &cfg.toastSeconds, 1.0f, 15.0f, "%.1f s");
                ImGui::SliderFloat(
                    "OCR prompt timeout",
                    &cfg.ocrPromptSeconds,
                    2.0f, 30.0f, "%.1f s");
                ImGui::SameLine();
                ImGui::TextDisabled(
                    "How long the Image Detected / Start OCR prompt remains open.");
                const char* toastFpsLabels[] = {"30", "60", "90", "120", "144", "165", "240"};
                const int toastFpsValues[] = {30, 60, 90, 120, 144, 165, 240};
                int toastFpsIndex = 3;
                for (int i=0;i<7;++i) if (cfg.toastFps == toastFpsValues[i]) toastFpsIndex=i;
                ImGui::TextUnformatted("Notification FPS"); ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                if (ImGui::Combo("##toast_fps", &toastFpsIndex, toastFpsLabels, 7))
                    cfg.toastFps = toastFpsValues[toastFpsIndex];
                ImGui::SameLine(); ImGui::TextDisabled("Only applies while the notification is visible.");
                ImGui::Checkbox("Acknowledge any clipboard change", &cfg.toastAcknowledgeClipboard);
                ImGui::SameLine(); ImGui::TextDisabled("Brief popup confirms clipboard watch is active.");
                if (ImGui::Checkbox("Play sound", &cfg.toastSound)) {
#ifdef _WIN32
                    gTraySoundEnabled = cfg.toastSound;
#elif defined(__APPLE__)
                    LogSiftMacTraySetSound(cfg.toastSound);
#endif
                    AppendActivityLog(appLog, "SOUND",
                        cfg.toastSound ? "Notification sound enabled from Settings."
                                       : "Notification sound disabled from Settings.");
                }
                const char* startSounds[] = {"Off", "Tick", "Soft", "Chime", "Pulse", "Sweep", "Ping", "Triple"};
                const char* endSounds[] = {"Off", "Soft", "Chime", "Success", "Attention", "Offline", "Pop", "Spark", "Low"};
                ImGui::TextUnformatted("Start"); ImGui::SameLine();
                ImGui::SetNextItemWidth(120); ImGui::Combo("##start_sound", &cfg.startSoundPreset, startSounds, 8);
                ImGui::SameLine();
                if (ImGui::Button("Test##start_sound")) PlaySynthPreset(cfg.startSoundPreset, true);
                ImGui::SameLine(); ImGui::TextDisabled("Plays when a recognized log begins scanning.");

                ImGui::TextUnformatted("Complete"); ImGui::SameLine();
                ImGui::SetNextItemWidth(120); ImGui::Combo("##end_sound", &cfg.endSoundPreset, endSounds, 9);
                ImGui::SameLine();
                if (ImGui::Button("Test##end_sound")) PlaySynthPreset(cfg.endSoundPreset, false);
                ImGui::SameLine(); ImGui::TextDisabled("Normal completion sound.");

                ImGui::TextUnformatted("Offline fallback"); ImGui::SameLine();
                ImGui::SetNextItemWidth(120); ImGui::Combo("##offline_sound", &cfg.offlineSoundPreset, endSounds, 9);
                ImGui::SameLine();
                if (ImGui::Button("Test##offline_sound")) PlaySynthPreset(cfg.offlineSoundPreset, false);
                ImGui::SameLine(); ImGui::TextDisabled("Distinct sound when the model fails and local filtering takes over.");

                ImGui::TextUnformatted("Failure"); ImGui::SameLine();
                ImGui::SetNextItemWidth(120); ImGui::Combo("##failure_sound", &cfg.failureSoundPreset, endSounds, 9);
                ImGui::SameLine();
                if (ImGui::Button("Test##failure_sound")) PlaySynthPreset(cfg.failureSoundPreset, false);
                ImGui::SameLine(); ImGui::TextDisabled("Used for hard failures that cannot fall back.");

                ImGui::TextUnformatted("Custom end sound"); ImGui::SameLine();
                ImGui::SetNextItemWidth(320);
                std::string shownSound = cfg.toastSoundFile.empty() ? "None (using procedural preset)" : cfg.toastSoundFile;
                ImGui::InputText("##custom_sound", &shownSound, ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                if (ImGui::Button("Browse...")) {
#ifdef _WIN32
                    const std::string picked = PickWaveFile(static_cast<HWND>(SDL_GetPointerProperty(
                        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr)));
                    if (!picked.empty()) cfg.toastSoundFile = picked;
#endif
                }
                ImGui::SameLine();
                ImGui::BeginDisabled(cfg.toastSoundFile.empty());
                if (ImGui::Button("Clear##sound")) cfg.toastSoundFile.clear();
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("Test##custom_end")) PlayEndSound(cfg);
                ImGui::SeparatorText("POPUP CONTENT");
                ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "Scanning");
                if (ImGui::BeginTable("##scan_popup_content", 3, ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableNextColumn(); ImGui::Checkbox("Source / log type##scan", &cfg.scanShowSource);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Model##scan", &cfg.scanShowModel);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Route / compute##scan", &cfg.scanShowRoute);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Progress / chunking##scan", &cfg.scanShowProgress);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Lines / words##scan", &cfg.scanShowPrefilterCounts);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Estimated tokens##scan", &cfg.scanShowEstimatedTokens);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Bytes / reduction##scan", &cfg.scanShowBytesReduction);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Elapsed time##scan", &cfg.scanShowElapsedTime);
                    ImGui::EndTable();
                }

                ImGui::Spacing();
                ImGui::TextColored(
                    ImVec4(0.28f, 0.82f, 1.00f, 1.0f),
                    "Image Detected");
                ImGui::TextDisabled(
                    "Shown for clipboard images when OCR is enabled and Auto-scan images is off.");
                if (ImGui::BeginTable(
                        "##image_detected_popup_content", 3,
                        ImGuiTableFlags_SizingStretchSame)) {
                    bool askBeforeOcr = !cfg.autoScanImages;
                    ImGui::TableNextColumn();
                    if (ImGui::Checkbox(
                            "Ask before OCR##ocr_prompt",
                            &askBeforeOcr)) {
                        cfg.autoScanImages = !askBeforeOcr;
                    }
                    ImGui::TableNextColumn();
                    ImGui::Checkbox(
                        "Image details##ocr_prompt",
                        &cfg.ocrPromptShowImageDetails);
                    ImGui::TableNextColumn();
                    ImGui::Checkbox(
                        "Timeout bar##ocr_prompt",
                        &cfg.ocrPromptShowTimeoutBar);
                    ImGui::EndTable();
                }
                ImGui::TextDisabled(
                    "Prompt timeout: %.1f s", cfg.ocrPromptSeconds);

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.30f, 0.90f, 0.48f, 1.0f), "Complete / Result");
                if (ImGui::BeginTable("##result_popup_content", 3, ImGuiTableFlags_SizingStretchSame)) {
                    ImGui::TableNextColumn(); ImGui::Checkbox("Diagnostic total##result", &cfg.resultShowDiagnosticTotal);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Source / log type##result", &cfg.resultShowSource);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Model##result", &cfg.resultShowModel);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Route / compute##result", &cfg.resultShowRoute);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Fallback notice##result", &cfg.resultShowFallbackNotice);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Lines / words##result", &cfg.resultShowPrefilterCounts);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Estimated tokens##result", &cfg.resultShowEstimatedTokens);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Real LLM tokens##result", &cfg.resultShowRealTokens);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Token speed##result", &cfg.resultShowTokenSpeed);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Bytes / reduction##result", &cfg.resultShowBytesReduction);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Completed time##result", &cfg.resultShowTime);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Auto-copy banner##result", &cfg.resultShowAutoCopy);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Included / questionable##result", &cfg.resultShowCounts);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Diagnostic preview##result", &cfg.resultShowPreview);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Dismiss timer bar##result", &cfg.resultShowLifetimeBar);
                    ImGui::EndTable();
                }
                ImGui::SetNextItemWidth(110);
                ImGui::SliderInt("Preview lines", &cfg.toastPreviewLines, 1, 10, "%d");
                ImGui::SameLine();
                ImGui::TextDisabled("Used when Diagnostic preview is enabled.");
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
