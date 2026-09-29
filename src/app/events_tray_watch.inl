// Main loop tray/menu commands and clipboard watch handling.
// Included by main.cpp; keep this module focused on this responsibility.

    while (running) {
        MaybeShutdownNotificationAudio();
#ifdef __APPLE__
        if (LogSiftMacTrayTakeOpen()) {
            reopenMainWindow();
        }
        if (LogSiftMacTrayTakeToggleWatch()) {
            cfg.watchClipboard=!cfg.watchClipboard;
            LogSiftMacTraySetWatch(cfg.watchClipboard);
            lastClipboardText.clear();
            status=cfg.watchClipboard ? "Clipboard watch enabled." : "Clipboard watch disabled.";
            SaveConfig(cfg);
            lastSavedConfig = ConfigToJson(cfg).dump();
            AppendActivityLog(appLog, "WATCH", status);
        }
        if (LogSiftMacTrayTakeToggleOcr()) {
            cfg.ocrEnabled = !cfg.ocrEnabled;
            LogSiftMacTraySetOcr(cfg.ocrEnabled);
            status = cfg.ocrEnabled ? "OCR enabled." : "OCR disabled.";
            SaveConfig(cfg);
            lastSavedConfig = ConfigToJson(cfg).dump();
            AppendActivityLog(appLog, "OCR", status);
        }
        if (LogSiftMacTrayTakeToggleAutoCopy()) {
            cfg.autoCopyResults = !cfg.autoCopyResults;
            LogSiftMacTraySetAutoCopy(cfg.autoCopyResults);
            status = cfg.autoCopyResults ? "Auto copy enabled." : "Auto copy disabled.";
            SaveConfig(cfg);
            lastSavedConfig = ConfigToJson(cfg).dump();
            AppendActivityLog(appLog, "AUTO-COPY", status);
        }
        if (LogSiftMacTrayTakeToggleSound()) {
            cfg.toastSound = !cfg.toastSound;
            LogSiftMacTraySetSound(cfg.toastSound);
            status = cfg.toastSound ? "Notification sound enabled." : "Notification sound disabled.";
            SaveConfig(cfg);
            lastSavedConfig = ConfigToJson(cfg).dump();
            AppendActivityLog(appLog, "SOUND", status);
        }
        if (LogSiftMacTrayTakeOpenLog()) {
            reopenMainWindow();
            showAppLog = true;
            AppendActivityLog(appLog, "UI", "Activity log opened from macOS menu bar.");
        }
        if (LogSiftMacTrayTakeCopy() && !output.empty()) {
            SetOwnedClipboardText(output, &lastClipboardText);
            status="Result copied from menu bar.";
            AppendActivityLog(appLog, "COPY", "Result copied from macOS menu bar.");
        }
        if (LogSiftMacTrayTakeQuit()) running=false;
#endif
#ifdef _WIN32
        MSG trayMsg{};
        while (PeekMessageW(&trayMsg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&trayMsg);
            DispatchMessageW(&trayMsg);
        }
        if (gTrayRestoreRequested) {
            gTrayRestoreRequested = false;
            reopenMainWindow();
        }
        if (gTrayWatchToggleRequested) {
            gTrayWatchToggleRequested = false;
            cfg.watchClipboard = !cfg.watchClipboard;
            gTrayWatchEnabled = cfg.watchClipboard;
            lastClipboardSequence = GetClipboardSequenceNumber();
            gClipboardUpdatePending = false;
            lastClipboardText.clear();
            status = cfg.watchClipboard ? "Clipboard watch enabled." : "Clipboard watch disabled.";
            SaveConfig(cfg);
            lastSavedConfig = ConfigToJson(cfg).dump();
            AppendActivityLog(appLog, "WATCH", status);
        }
        if (gTrayOcrToggleRequested) {
            gTrayOcrToggleRequested = false;
            cfg.ocrEnabled = !cfg.ocrEnabled;
            gTrayOcrEnabled = cfg.ocrEnabled;
            status = cfg.ocrEnabled ? "OCR enabled." : "OCR disabled.";
            SaveConfig(cfg);
            lastSavedConfig = ConfigToJson(cfg).dump();
            AppendActivityLog(appLog, "OCR", status);
        }
        if (gTrayAutoCopyToggleRequested) {
            gTrayAutoCopyToggleRequested = false;
            cfg.autoCopyResults = !cfg.autoCopyResults;
            gTrayAutoCopyEnabled = cfg.autoCopyResults;
            status = cfg.autoCopyResults ? "Auto copy enabled." : "Auto copy disabled.";
            SaveConfig(cfg);
            lastSavedConfig = ConfigToJson(cfg).dump();
            AppendActivityLog(appLog, "AUTO-COPY", status);
        }
        if (gTraySoundToggleRequested) {
            gTraySoundToggleRequested = false;
            cfg.toastSound = !cfg.toastSound;
            gTraySoundEnabled = cfg.toastSound;
            status = cfg.toastSound ? "Notification sound enabled." : "Notification sound disabled.";
            SaveConfig(cfg);
            lastSavedConfig = ConfigToJson(cfg).dump();
            AppendActivityLog(appLog, "SOUND", status);
        }
        if (gTrayOpenLogRequested) {
            gTrayOpenLogRequested = false;
            reopenMainWindow();
            showAppLog = true;
            AppendActivityLog(appLog, "UI", "Activity log opened from Windows tray.");
        }
        if (gTrayCopyRequested) {
            gTrayCopyRequested = false;
            if (!output.empty()) {
                SetOwnedClipboardText(output, &lastClipboardText);
                lastClipboardSequence = GetClipboardSequenceNumber();
                gClipboardUpdatePending = false;
                status = "Result copied from tray.";
                AppendActivityLog(appLog, "COPY", "Result copied from Windows tray.");
            }
        }
        if (gTrayExitRequested) running = false;
#endif
#ifdef _WIN32
        // When hidden and genuinely idle, block the thread instead of building/rendering
        // invisible ImGui frames. WM_CLIPBOARDUPDATE and tray messages wake this instantly.
        const bool hiddenNow = (SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) != 0;
        const bool asyncNow = busy || checkingHealth || benchmarking || loadingModels || applyingCompute;
        const bool toastWindowVisible =
            toastWindow && (SDL_GetWindowFlags(toastWindow) & SDL_WINDOW_HIDDEN) == 0;
        const bool toastNow =
            !toastText.empty() || toastProcessing || toastWindowVisible;
        if (hiddenNow && !asyncNow && !toastNow && !gClipboardUpdatePending &&
            !gTrayRestoreRequested && !gTrayWatchToggleRequested &&
            !gTrayOcrToggleRequested && !gTrayAutoCopyToggleRequested && !gTraySoundToggleRequested &&
            !gTrayOpenLogRequested &&
            !gTrayCopyRequested && !gTrayExitRequested) {
            MsgWaitForMultipleObjectsEx(0, nullptr, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
            continue;
        }
#endif
        const auto now = std::chrono::steady_clock::now();
#ifdef _WIN32
        const bool clipboardTriggered = cfg.watchClipboard && !busy && gClipboardUpdatePending;
#else
        bool clipboardTriggered = false;
        if (cfg.watchClipboard && !busy &&
            now - lastClipboardCheck >= std::chrono::milliseconds(350)) {
            lastClipboardCheck = now;
#ifdef __APPLE__
            const long long macClipboardChangeCount = LogSiftMacClipboardChangeCount();
            if (macClipboardChangeCount != lastMacClipboardChangeCount) {
                lastMacClipboardChangeCount = macClipboardChangeCount;
                clipboardTriggered = true;
            }
#else
            clipboardTriggered = true;
#endif
        }
#endif
        if (clipboardTriggered) {
#ifdef _WIN32
            gClipboardUpdatePending = false;
            const DWORD clipboardSequence = GetClipboardSequenceNumber();
            if (clipboardSequence == lastClipboardSequence) continue;
            lastClipboardSequence = clipboardSequence;
#endif
            // Clipboard activity is enough to opportunistically recheck an
            // offline endpoint, but image bytes stay untouched until that check
            // has confirmed the model is reachable again.
            const bool modelStateNeedsRecheck =
                connectionStage == ConnectionStage::Unreachable ||
                connectionStage == ConnectionStage::ModelsFailed ||
                connectionStage == ConnectionStage::BenchmarkFailed;
            if (modelStateNeedsRecheck && !checkingHealth)
                startHealthCheck(false, false);

            std::string clip;
            std::string clipboardSourceKind = "Clipboard";
            ClipboardImage clipboardImage;
            bool clipboardHasImage = false;
            bool clipboardImageSuppressed = false;
            const bool modelReadyForImageWatch =
                connectionStage == ConnectionStage::Ready &&
                !checkingHealth && !loadingModels && !benchmarking &&
                !applyingCompute;

            if (!modelReadyForImageWatch) {
                // Detect only the clipboard format here; do not fetch/decode image
                // bytes until the model has been confirmed ready.
                clipboardImageSuppressed =
                    SDL_HasClipboardData("image/png") ||
                    SDL_HasClipboardData("image/jpeg") ||
                    SDL_HasClipboardData("image/jpg");
#ifdef _WIN32
                clipboardImageSuppressed =
                    clipboardImageSuppressed ||
                    IsClipboardFormatAvailable(CF_DIB) ||
                    IsClipboardFormatAvailable(CF_DIBV5);
#endif
            }
#ifdef _WIN32
            if (IsClipboardFormatAvailable(CF_HDROP) && OpenClipboard(nullptr)) {
                HDROP drop = static_cast<HDROP>(GetClipboardData(CF_HDROP));
                if (drop && DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0) > 0) {
                    wchar_t path[MAX_PATH]{};
                    if (DragQueryFileW(drop, 0, path, MAX_PATH) > 0) {
                        std::filesystem::path p(path);
                        std::string ext = p.extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(),
                            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

                        if (ext == ".log" || ext == ".txt") {
                            std::ifstream lf(p, std::ios::binary);
                            if (lf) {
                                std::ostringstream ss;
                                ss << lf.rdbuf();
                                clip = ss.str();
                                clipboardSourceKind = "File";
                            }
                        } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg") {
                            if (!modelReadyForImageWatch) {
                                // Do not read image bytes or enter the OCR flow while the
                                // selected model is not known to be reachable and ready.
                                clipboardImageSuppressed = true;
                            } else {
                                std::ifstream imageFile(p, std::ios::binary);
                                if (imageFile) {
                                    clipboardImage.bytes.assign(
                                        std::istreambuf_iterator<char>(imageFile),
                                        std::istreambuf_iterator<char>());
                                    if (!clipboardImage.bytes.empty()) {
                                        clipboardImage.mimeType =
                                            ext == ".png" ? "image/png" : "image/jpeg";
                                        clipboardHasImage = true;
                                    }
                                }
                            }
                        }
                    }
                }
                CloseClipboard();
            }
#endif
            if (!clipboardHasImage && clip.empty() &&
                modelReadyForImageWatch) {
                clipboardHasImage = GetClipboardImage(clipboardImage);
            }

            // An image copied while the model is offline/checking is intentionally
            // ignored. Do not decode it, show the OCR prompt, or reinterpret a
            // copied image-file path as ordinary clipboard text.
            if (clipboardImageSuppressed)
                continue;

            if (clipboardHasImage) {
                AppendActivityLog(appLog, "CLIPBOARD",
                    "Clipboard image detected (" + clipboardImage.mimeType + ", " +
                    std::to_string(clipboardImage.bytes.size()) + " bytes).");

                if (!cfg.ocrEnabled) {
                    status = "Clipboard image ignored - OCR is disabled.";
                    AppendActivityLog(appLog, "OCR", status);
                } else if (!visionSupportKnown || !visionSupported) {
                    beginOcrScan(std::move(clipboardImage), "OCR", "Clipboard image");
                } else if (cfg.autoScanImages) {
                    beginOcrScan(std::move(clipboardImage), "OCR", "Clipboard image");
                } else {
                    pendingOcrImage = std::move(clipboardImage);
                    pendingOcrImageReady = true;

                    output.clear();
                    questionableOutput.clear();
                    lastInputBytes = 0;
                    lastFilteredBytes = 0;
                    stats = {};
                    inputSourceKind = "OCR";
                    stats.sourceKind = inputSourceKind;
                    stats.logType = "Image / OCR";
                    stats.profile = "Vision OCR";
                    stats.model = cfg.model;
                    stats.compute =
                        cfg.computeMode == 1 ? "GPU max" :
                        cfg.computeMode == 2 ? "CPU" : "Auto";
                    stats.inputBytes = pendingOcrImage.bytes.size();
                    stats.route = "Awaiting OCR";
                    stats.ocr.present = true;
                    stats.ocr.mimeType = pendingOcrImage.mimeType;
                    stats.ocr.imageBytes = pendingOcrImage.bytes.size();

                    status = "Clipboard image detected - waiting for OCR confirmation.";
                    toastText = "Image detected";
                    toastProcessing = false;
                    toastOutcome = ToastOutcome::OcrPrompt;
                    toastAutoCopied = false;
                    toastShownAt = now;
                    toastUntil = now + std::chrono::milliseconds(
                        static_cast<int>(cfg.ocrPromptSeconds * 1000.0f));
                    if (cfg.toastSound) PlaySynthPreset(cfg.startSoundPreset, true);
                    toastSoundPlayed = true;
                    AppendActivityLog(appLog, "OCR",
                        "Clipboard image detected; waiting for Start OCR.");
                }
            }

            if (!clipboardHasImage && clip.empty()) {
                char* clipboard = SDL_GetClipboardText();
                clip = clipboard ? clipboard : "";
                if (clipboard) SDL_free(clipboard);
            }
            if (!clipboardHasImage && !clip.empty()
#ifndef _WIN32
                && clip != lastClipboardText
#endif
            ) {
                AppendActivityLog(appLog, "CLIPBOARD",
                    "Clipboard change detected (" + std::to_string(clip.size()) + " bytes).");
                lastClipboardText = clip;
                ++requestGeneration;
                output.clear();
                questionableOutput.clear();
                lastInputBytes = 0;
                lastFilteredBytes = 0;
                stats = {};
                inputSourceKind = clipboardSourceKind;
                stats.sourceKind = inputSourceKind;
                    if (cfg.toastAcknowledgeClipboard) {
                    toastText = "Clipboard detected";
                    toastProcessing = false;
                    toastOutcome = ToastOutcome::Processing;
                    toastSoundPlayed = true;
                    toastAutoCopied = false; // acknowledgement is visual; completion sound remains separate.
                    toastShownAt = now;
                    toastUntil = now + std::chrono::milliseconds(1800);
                }
                const std::string filtered = PreFilter(clip, cfg);
                const bool parseable = LooksLikeUnrealLog(clip) || LooksLikeStructuredBuildDiagnostics(filtered) || LooksLikeGenericLog(clip);
                if (parseable && filtered.empty()) {
                    input = clip;
                    output.clear();
                    questionableOutput.clear();
                    stats.logType = DetectLogType(input);
                    stats.profile = ProfileName(input, cfg);
                    stats.model = cfg.model;
                    stats.compute = cfg.computeMode == 1 ? "GPU max" : cfg.computeMode == 2 ? "CPU" : "Auto";
                    stats.inputBytes = input.size();
                    stats.filteredBytes = 0;
                    {
                        const auto [lines, words] = HumanTextStats(input);
                        stats.inputLines = lines;
                        stats.inputWords = words;
                        stats.filteredLines = 0;
                        stats.filteredWords = 0;
                        stats.estimatedInputTokens = EstimateTokenCount(input);
                        stats.estimatedFilteredTokens = 0;
                    }
                    stats.seconds = 0.0;
                    stats.route = "Local prefilter";
                    status = "Clipboard log scanned - no diagnostics found.";
                    AppendActivityLog(appLog, "SIFT", "Clipboard scan complete: no actionable diagnostics.");
                    toastText = "Nothing found";
                    toastProcessing = false;
                    toastOutcome = ToastOutcome::Empty;
                    toastSoundPlayed = false;
                    toastAutoCopied = false;
                    toastShownAt = now;
                    toastUntil = now + std::chrono::milliseconds(static_cast<int>(cfg.toastSeconds * 1000.0f));
                    recordRecentRun();
                } else if (parseable && !filtered.empty()) {
                    input = clip;
                    const DiagnosticSplit split = LooksLikeUnrealLog(input) && (cfg.profileId=="auto" || cfg.profileId=="unreal") ? SplitUnrealDiagnostics(input, cfg) : SplitWithProfile(input, cfg);
                    questionableOutput = ApplyOutputPreferences(split.questionable, cfg);
                    const std::string previewFiltered = !split.included.empty() ? split.included : filtered;
                    lastInputBytes = input.size();
                    lastFilteredBytes = previewFiltered.size();
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
                    stats.route = (cfg.preferFastPath && LooksLikeStructuredBuildDiagnostics(previewFiltered)) ? "Deterministic fast path" : "LLM";
                    requestStarted = now;
                    toastText = "Processing";
                    toastProcessing = true;
                    toastOutcome = ToastOutcome::Processing;
                    toastSoundPlayed = false;
                    toastAutoCopied = false;
                    toastShownAt = now;
                    toastUntil = now + std::chrono::hours(1);
                    if (cfg.toastSound) PlaySynthPreset(cfg.startSoundPreset, true);
                    if (cfg.preferFastPath && LooksLikeStructuredBuildDiagnostics(previewFiltered)) {
                        output = ApplyOutputPreferences(FastStructuredResult(previewFiltered), cfg);
                        stats.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - requestStarted).count();
                        const bool autoCopied = MaybeAutoCopyResult(cfg, output, lastClipboardText);
                        if (autoCopied) markOwnClipboardWrite();
                        toastAutoCopied = autoCopied;
                        status = autoCopied ? "Clipboard log parsed and result auto-copied." : "Clipboard log parsed.";
                        AppendActivityLog(appLog, "SUCCESS",
                            "Deterministic clipboard sift completed with " +
                            std::to_string(DiagnosticEntries(output).size()) + " diagnostics.");
                        if (autoCopied)
                            AppendActivityLog(appLog, "AUTO-COPY", "Actionable clipboard result auto-copied.");
                        toastText = "Complete";
                        toastProcessing = false;
                        toastOutcome = output.empty() || output == "NO_DIAGNOSTICS\n" ? ToastOutcome::Empty : ToastOutcome::Success;
                        toastShownAt = std::chrono::steady_clock::now();
                        toastUntil = std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<int>(cfg.toastSeconds * 1000.0f));
                        recordRecentRun();

                    } else {
                        const Config capturedCfg = cfg;
                        const std::string capturedInput = input;
                        const std::string capturedPrompt = prompt;
                        status = "Clipboard log detected - sifting...";
                        AppendActivityLog(appLog, "LLM", "Clipboard log sent to model for sifting.");
                        activeRequestGeneration = requestGeneration;
                        activeProgress = std::make_shared<SiftProgress>();
                        const int initialChunks = static_cast<int>(ChunkModelInput(previewFiltered).size());
                        activeProgress->total = std::max(1, initialChunks);
                        activeProgress->chunking = initialChunks > 1;
                        if (initialChunks > 1) {
                            status = "Large clipboard log - chunking " + std::to_string(initialChunks) +
                                " model chunks; this may take longer.";
                            AppendActivityLog(appLog, "CHUNK",
                                "Large clipboard log split into " + std::to_string(initialChunks) + " model chunks.");
                        }
                        busy = true;
                        request = LaunchSiftTask(
                            [capturedCfg, capturedInput, capturedPrompt, progress = activeProgress] {
                                return Send(capturedCfg, capturedInput, capturedPrompt, progress);
                            });
                    }
                }
            }
        }

        SDL_Event event{};
        bool hasWaitingEvent = false;
#ifdef __APPLE__
        // With both windows hidden, SDL can wait for native menu/window events.
        // Wake periodically only to sample NSPasteboard (which has no change event).
        const bool mainHiddenForWait =
            (SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) != 0;
        const bool toastVisibleForWait =
            toastWindow && (SDL_GetWindowFlags(toastWindow) & SDL_WINDOW_HIDDEN) == 0;
        const bool asyncForWait =
            busy || checkingHealth || benchmarking || loadingModels || applyingCompute;
        const bool toastForWait =
            !toastText.empty() || toastProcessing || toastVisibleForWait;
        if (running && mainHiddenForWait && !asyncForWait && !toastForWait) {
            hasWaitingEvent = SDL_WaitEventTimeout(
                &event, cfg.watchClipboard ? 350 : 500);
            if (!hasWaitingEvent) continue;
        }
#endif
