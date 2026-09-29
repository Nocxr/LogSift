// SDL events plus async model/benchmark/request completion.
// Included by main.cpp; keep this module focused on this responsibility.

        while (hasWaitingEvent || SDL_PollEvent(&event)) {
            hasWaitingEvent = false;
            const SDL_WindowID toastWindowId = toastWindow ? SDL_GetWindowID(toastWindow) : 0;
            if (!toastProcessing && toastTimerPaused && toastWindowId != 0) {
                const bool toastLostFocus =
                    event.type == SDL_EVENT_WINDOW_FOCUS_LOST &&
                    event.window.windowID == toastWindowId;
                const bool clickedAnotherAppWindow =
                    event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                    event.button.windowID != 0 &&
                    event.button.windowID != toastWindowId;
                if (toastLostFocus || clickedAnotherAppWindow)
                    toastResumeRequested = true;
            }

            if (toastContext && toastWindow && event.window.windowID == SDL_GetWindowID(toastWindow)) {
                ImGui::SetCurrentContext(toastContext);
                ImGui_ImplSDL3_ProcessEvent(&event);
                ImGui::SetCurrentContext(mainContext);
            } else {
                ImGui::SetCurrentContext(mainContext);
                ImGui_ImplSDL3_ProcessEvent(&event);
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                const SDL_WindowID mainWindowId = SDL_GetWindowID(window);
                if (event.window.windowID == mainWindowId) {
#if defined(_WIN32) || defined(__APPLE__)
                    // The app is tray/menu-bar resident. Handle the main window's
                    // close request directly so it still hides when the separate
                    // notification window is currently open.
                    SDL_HideWindow(window);
                    AppendActivityLog(
                        appLog, "UI", "Main window hidden from title-bar close.");
#else
                    running = false;
#endif
                } else if (toastWindowId != 0 &&
                           event.window.windowID == toastWindowId) {
                    // Native close/Alt+F4 on the notification should behave like
                    // its in-popup X: hide only the popup and leave any active sift
                    // running. Discard a pending OCR confirmation because there is
                    // no longer a visible way to accept it.
                    if (toastOutcome == ToastOutcome::OcrPrompt) {
                        pendingOcrImage = {};
                        pendingOcrImageReady = false;
                    }
                    toastText.clear();
                    SDL_HideWindow(toastWindow);
                    AppendActivityLog(
                        appLog, "UI", "Notification closed from window controls.");
                }
            }

            if (event.type == SDL_EVENT_QUIT) {
#if defined(_WIN32) || defined(__APPLE__)
                SDL_HideWindow(window);
#else
                running = false;
#endif
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_GRAVE && !ImGui::GetIO().WantTextInput) showAppLog = !showAppLog;
            if (event.type == SDL_EVENT_DROP_FILE) {
                ClipboardImage droppedImage;
                std::string droppedStatus;
                if (LoadImageFile(event.drop.data, droppedImage, droppedStatus)) {
                    status = droppedStatus;
                    AppendActivityLog(appLog, "FILE", status);
                    beginOcrScan(std::move(droppedImage), "File", "Dropped image");
                } else if (!droppedStatus.empty()) {
                    status = droppedStatus;
                    AppendActivityLog(appLog, "FILE", status);
                } else if (LoadFile(event.drop.data, input, status)) {
                    AppendActivityLog(appLog, "FILE", status);
                    output.clear();
                    questionableOutput.clear();
                    lastInputBytes = lastFilteredBytes = 0;
                    stats = {};
                    inputSourceKind = "File";
                    stats.sourceKind = inputSourceKind;
                }
            }
        }

        if (busy && request.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            lastResponseSeconds = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - requestStarted).count();
            if (activeRequestGeneration != requestGeneration) {
                try { (void)request.get(); } catch (...) {}
                busy = false;
            } else {
                try {
                    const SiftResult result = request.get();

                    if (result.sourceWasImage) {
                        if (inputSourceKind != "File")
                            inputSourceKind = "OCR";
                        input = result.sourceText;
                        stats.sourceKind = inputSourceKind;
                        stats.logType = "Image / OCR";
                        stats.profile = input.empty() ? "Vision OCR" : ProfileName(input, cfg);
                        stats.model = cfg.model;
                        stats.compute = cfg.computeMode == 1 ? "GPU max" : cfg.computeMode == 2 ? "CPU" : "Auto";
                        stats.inputBytes = input.size();
                        const std::string ocrFiltered = PreFilter(input, cfg);
                        stats.filteredBytes = ocrFiltered.size();
                        const auto [ocrLines, ocrWords] = HumanTextStats(input);
                        const auto [ocrFilteredLines, ocrFilteredWords] = HumanTextStats(ocrFiltered);
                        stats.inputLines = ocrLines;
                        stats.inputWords = ocrWords;
                        stats.filteredLines = ocrFilteredLines;
                        stats.filteredWords = ocrFilteredWords;
                        stats.estimatedInputTokens = EstimateTokenCount(input);
                        stats.estimatedFilteredTokens = EstimateTokenCount(ocrFiltered);
                        lastInputBytes = stats.inputBytes;
                        lastFilteredBytes = stats.filteredBytes;
                        stats.ocr = result.ocr;
                        if (stats.ocr.present) {
                            const auto [ocrOutputLines, ocrOutputWords] =
                                HumanTextStats(result.sourceText);
                            stats.ocr.outputBytes = result.sourceText.size();
                            stats.ocr.outputLines = ocrOutputLines;
                            stats.ocr.outputWords = ocrOutputWords;
                        }
                    }

                    output = ApplyOutputPreferences(result.text, cfg);
                    stats.route = result.route;
                    stats.promptTokens = result.promptTokens;
                    stats.completionTokens = result.completionTokens;
                    stats.promptTokensPerSecond = result.promptTokensPerSecond;
                    stats.completionTokensPerSecond = result.completionTokensPerSecond;
                    if (stats.promptTokens > 0 && stats.seconds > 0.0)
                        stats.estimatedPromptTokensPerSecond = static_cast<double>(stats.promptTokens) / stats.seconds;

                    health = result.visionFailure
                        ? "Online / vision response issue"
                        : result.usedLocalFallback
                            ? "Online / response issue"
                            : "Online - model responded";
                    connectionStage = ConnectionStage::Ready;

                    const bool autoCopied = MaybeAutoCopyResult(cfg, output, lastClipboardText);
                    if (autoCopied) markOwnClipboardWrite();
                    toastAutoCopied = autoCopied;
                    if (result.visionFailure) {
                        status = "Vision/OCR failed. " + result.note;
                        AppendActivityLog(appLog, "OCR-FAIL", status);
                    } else if (result.usedLocalFallback) {
                        status = std::string("Model online - local filter used. ") + result.note;
                        if (autoCopied) status += " Result auto-copied.";
                        AppendActivityLog(appLog, "MODEL-FALLBACK",
                            "Model responded but local fallback was used. " + result.note);
                    } else {
                        status = autoCopied ? "Done - result auto-copied." : "Done.";
                        AppendActivityLog(appLog, "SUCCESS",
                            "Model sift completed via " + result.route + " with " +
                            std::to_string(DiagnosticEntries(output).size()) + " diagnostics in " +
                            std::to_string(lastResponseSeconds) + " s.");
                    }
                    if (autoCopied)
                        AppendActivityLog(appLog, "AUTO-COPY", "Actionable sift result auto-copied.");

                    if (cfg.watchClipboard || result.sourceWasImage) {
                        toastText = result.visionFailure
                            ? "OCR failed"
                            : result.usedLocalFallback ? "Model fallback" : "Complete";
                        toastProcessing = false;
                        toastOutcome = result.visionFailure
                            ? ToastOutcome::Failure
                            : result.usedLocalFallback
                                ? ToastOutcome::ModelFallback
                                : (output.empty() || output == "NO_DIAGNOSTICS\n"
                                    ? ToastOutcome::Empty
                                    : ToastOutcome::Success);
                        toastSoundPlayed = false;
                        toastShownAt = std::chrono::steady_clock::now();
                        toastUntil = std::chrono::steady_clock::now() +
                            std::chrono::milliseconds(static_cast<int>(cfg.toastSeconds * 1000.0f));
                    }
                } catch (const std::exception& e) {
                    const bool endpointUnavailable = IsEndpointUnavailableError(e.what());
                    const DiagnosticSplit fallbackSplit =
                        LooksLikeUnrealLog(input) && (cfg.profileId == "auto" || cfg.profileId == "unreal")
                            ? SplitUnrealDiagnostics(input, cfg)
                            : SplitWithProfile(input, cfg);
                    questionableOutput = ApplyOutputPreferences(fallbackSplit.questionable, cfg);
                    const std::string prefiltered = PreFilter(input, cfg);
                    const std::string fallbackCandidate =
                        !fallbackSplit.included.empty() ? fallbackSplit.included : prefiltered;
                    output = ApplyOutputPreferences(
                        LooksLikeStructuredBuildDiagnostics(fallbackCandidate)
                            ? FastStructuredResult(fallbackCandidate)
                            : DedupeLines(fallbackCandidate), cfg);
                    stats.filteredBytes = fallbackCandidate.size();
                    {
                        const auto [filteredLineCount, filteredWordCount] = HumanTextStats(fallbackCandidate);
                        stats.filteredLines = filteredLineCount;
                        stats.filteredWords = filteredWordCount;
                        stats.estimatedFilteredTokens = EstimateTokenCount(fallbackCandidate);
                    }
                    stats.route = endpointUnavailable ? "Offline fallback" : "Model fallback";
                    stats.promptTokens = 0;
                    stats.completionTokens = 0;
                    stats.promptTokensPerSecond = 0.0;
                    stats.completionTokensPerSecond = 0.0;
                    stats.estimatedPromptTokensPerSecond = 0.0;

                    if (endpointUnavailable) {
                        health = "Offline / unreachable";
                        connectionStage = ConnectionStage::Unreachable;
                    } else {
                        // The endpoint responded; never label a response-quality failure as offline.
                        health = "Online / response issue";
                        connectionStage = ConnectionStage::Ready;
                    }

                    const bool autoCopied = MaybeAutoCopyResult(cfg, output, lastClipboardText);
                    if (autoCopied) markOwnClipboardWrite();
                    toastAutoCopied = autoCopied;

                    const char* fallbackName = endpointUnavailable ? "Offline fallback" : "Model fallback";
                    status = std::string(fallbackName) +
                        (autoCopied ? " - local result auto-copied. " : " - local filter used. ") +
                        e.what();
                    AppendActivityLog(appLog,
                        endpointUnavailable ? "OFFLINE" : "MODEL-FAIL",
                        std::string(fallbackName) + ": " + e.what());
                    if (autoCopied)
                        AppendActivityLog(appLog, "AUTO-COPY", "Fallback diagnostics auto-copied.");

                    if (cfg.toastSound) {
                        PlaySynthPreset(
                            endpointUnavailable ? cfg.offlineSoundPreset : cfg.failureSoundPreset,
                            false);
                        toastSoundPlayed = true;
                    }

                    if (cfg.watchClipboard) {
                        toastText = endpointUnavailable ? "Offline fallback" : "Model fallback";
                        toastProcessing = false;
                        toastOutcome = endpointUnavailable
                            ? ToastOutcome::OfflineFallback
                            : ToastOutcome::ModelFallback;
                        toastSoundPlayed = cfg.toastSound;
                        toastShownAt = std::chrono::steady_clock::now();
                        toastUntil = toastShownAt + std::chrono::milliseconds(
                            static_cast<int>(cfg.toastSeconds * 1000.0f));
                    }
                }
                stats.seconds = lastResponseSeconds;
                recordRecentRun();
                busy = false;
            }
        }
        if (checkingHealth && healthRequest.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            bool online = false;
            bool modelAvailable = false;
            try {
                const ModelHealthResult healthResult = healthRequest.get();
                health = healthResult.status;
                online = healthResult.online;
                modelAvailable = healthResult.modelAvailable;
                visionSupportKnown = healthResult.visionChecked;
                visionSupported = healthResult.visionSupported;
                visionStatus = !healthResult.visionChecked
                    ? "Unknown"
                    : healthResult.visionSupported ? "Supported" : "Not supported";
            } catch (const std::exception& e) {
                health = std::string("Offline / unreachable: ") + e.what();
                visionSupportKnown = false;
                visionSupported = false;
                visionStatus = "Unknown";
            }
            checkingHealth = false;

            if (!online) {
                AppendActivityLog(appLog, "HEALTH", "Model endpoint unreachable: " + health);
                connectionStage = ConnectionStage::Unreachable;

                // A pending OCR confirmation is no longer actionable. Remove it
                // immediately instead of leaving an Image Detected popup visible
                // while the model is offline.
                if (toastOutcome == ToastOutcome::OcrPrompt) {
                    pendingOcrImage = {};
                    pendingOcrImageReady = false;
                    toastText.clear();
                    toastProcessing = false;
                    if (toastWindow)
                        SDL_HideWindow(toastWindow);
                }

                benchmarkStatus = startupConnectionSequence
                    ? "Startup benchmark skipped - model unreachable"
                    : benchmarkStatus;
                if (warnOnHealthFailure)
                    status = "WARNING: model endpoint is unreachable; local filtering will be used.";
                startupConnectionSequence = false;
            } else if (startupConnectionSequence) {
                AppendActivityLog(appLog, "HEALTH", "Model endpoint reachable; loading model list.");
                connectionStage = ConnectionStage::LoadingModels;
                benchmarkStatus = "Loading model list...";
                loadingModels = true;
                const Config capturedCfg = cfg;
                modelListRequest = LaunchBackgroundTask([capturedCfg] { return ListModels(capturedCfg); });
            } else if (!modelAvailable) {
                connectionStage = ConnectionStage::ModelsFailed;
                AppendActivityLog(appLog, "HEALTH",
                    "Model endpoint reachable, but selected model is unavailable.");
                if (warnOnHealthFailure)
                    status = "Model endpoint reachable, but selected model is unavailable.";
            } else {
                connectionStage = ConnectionStage::Ready;
                AppendActivityLog(appLog, "HEALTH",
                    "Model endpoint and selected model reachable; Vision/OCR: " + visionStatus + ".");
                if (warnOnHealthFailure)
                    status = "Model reachable. Vision/OCR: " + visionStatus + ".";
            }
            warnOnHealthFailure = false;
        }

        if (loadingModels && modelListRequest.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            bool modelsLoaded = false;
            try {
                availableModels = modelListRequest.get();
                modelsLoaded = !availableModels.empty();
            } catch (...) {
                availableModels.clear();
            }
            loadingModels = false;

            if (!modelsLoaded) {
                AppendActivityLog(appLog, "MODELS", "Model discovery failed or returned no models.");
                connectionStage = ConnectionStage::ModelsFailed;
                if (startupConnectionSequence)
                    benchmarkStatus = "Startup benchmark skipped - model list unavailable";
                startupConnectionSequence = false;
            } else if (startupConnectionSequence) {
                AppendActivityLog(appLog, "MODELS",
                    "Discovered " + std::to_string(availableModels.size()) + " model(s).");
                if (std::find(availableModels.begin(), availableModels.end(), cfg.model) ==
                    availableModels.end()) {
                    cfg.model = availableModels.front();
                }
                connectionStage = ConnectionStage::Benchmarking;
                benchmarkStatus = "Benchmarking selected model...";
                AppendActivityLog(appLog, "BENCH", "Startup benchmark started for " + cfg.model + ".");
                benchmarking = true;
                const Config capturedCfg = cfg;
                benchmarkRequest = LaunchBackgroundTask([capturedCfg] { return Benchmark(capturedCfg); });
            } else {
                connectionStage = ConnectionStage::Ready;
                AppendActivityLog(appLog, "MODELS",
                    "Model refresh complete: " + std::to_string(availableModels.size()) + " model(s) available.");
            }
        }

        if (applyingCompute && computeRequest.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            try {
                computeStatus = computeRequest.get();
                AppendActivityLog(appLog, "COMPUTE", computeStatus);
            }
            catch (const std::exception& e) {
                computeStatus = std::string("Compute change failed: ") + e.what();
                AppendActivityLog(appLog, "COMPUTE-FAIL", computeStatus);
            }
            applyingCompute = false;
        }

        if (benchmarking && benchmarkRequest.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            try {
                benchmarkStatus = benchmarkRequest.get();
                AppendActivityLog(appLog, "BENCH", benchmarkStatus);
                connectionStage = ConnectionStage::Ready;
                health = "Online - model available";
                if (startupConnectionSequence)
                    status = "Ready - health check, model discovery, and benchmark complete.";
            } catch (const std::exception& e) {
                benchmarkStatus = std::string("Benchmark failed: ") + e.what();
                AppendActivityLog(appLog, "BENCH-FAIL", benchmarkStatus);
                connectionStage = ConnectionStage::BenchmarkFailed;
                health = "Online - benchmark failed";
            }
            benchmarking = false;
            startupConnectionSequence = false;
        }

