// Popup sound/timer lifecycle and visibility state.
// Included by main.cpp; keep this module focused on this responsibility.

        if (!toastProcessing && toastOutcome != ToastOutcome::OcrPrompt &&
            !toastText.empty() && cfg.toastSound && !toastSoundPlayed) {
            toastSoundPlayed = true;
            if (toastOutcome == ToastOutcome::OfflineFallback) {
                PlaySynthPreset(cfg.offlineSoundPreset, false);
            } else if (toastOutcome == ToastOutcome::ModelFallback) {
                PlaySynthPreset(cfg.failureSoundPreset, false);
            } else {
                PlayEndSound(cfg, toastOutcome == ToastOutcome::Failure);
            }
        }
        const auto toastTimerNow = std::chrono::steady_clock::now();

        // Every newly-created notification starts with a clean hover-hold state.
        if (toastShownAt != toastPauseToastShownAt) {
            toastPauseToastShownAt = toastShownAt;
            toastTimerPaused = false;
            toastMouseWasOver = false;
            toastResumeRequested = false;
            toastPausedRemaining = {};
            toastMouseLeftAt = {};
        }

        if (!toastProcessing && !toastText.empty() && toastWindow) {
            const bool mouseOverToast =
                (SDL_GetWindowFlags(toastWindow) & SDL_WINDOW_MOUSE_FOCUS) != 0;

            if (mouseOverToast) {
                if (!toastTimerPaused) {
                    toastPausedRemaining = std::max(
                        std::chrono::steady_clock::duration::zero(),
                        toastUntil - toastTimerNow);
                    toastTimerPaused = true;
                }
                toastMouseLeftAt = {};
                toastMouseWasOver = true;
            } else if (toastTimerPaused) {
                if (toastResumeRequested) {
                    toastUntil = toastTimerNow + toastPausedRemaining;
                    toastTimerPaused = false;
                    toastMouseLeftAt = {};
                    toastResumeRequested = false;
                } else {
                    if (toastMouseWasOver && toastMouseLeftAt.time_since_epoch().count() == 0)
                        toastMouseLeftAt = toastTimerNow;

                    constexpr auto kToastHoverGrace = std::chrono::seconds(2);
                    if (toastMouseLeftAt.time_since_epoch().count() != 0 &&
                        toastTimerNow - toastMouseLeftAt >= kToastHoverGrace) {
                        toastUntil = toastTimerNow + toastPausedRemaining;
                        toastTimerPaused = false;
                        toastMouseLeftAt = {};
                    }
                }
                toastMouseWasOver = false;
            }
        }

        const bool toastActive =
            !toastText.empty() &&
            (toastProcessing || toastTimerPaused || toastTimerNow < toastUntil);
        if (!toastActive && !toastText.empty()) {
            if (toastOutcome == ToastOutcome::OcrPrompt) {
                pendingOcrImage = {};
                pendingOcrImageReady = false;
                status = "OCR image prompt timed out.";
                AppendActivityLog(appLog, "OCR", status);
            }
            toastText.clear();
        }

        if (toastContext && toastWindow && toastRenderer) {
            if (toastActive) {
                const bool toastWasHidden =
                    (SDL_GetWindowFlags(toastWindow) & SDL_WINDOW_HIDDEN) != 0;

                int displayCount = 0;
                SDL_DisplayID* displays = SDL_GetDisplays(&displayCount);
                SDL_DisplayID display = (displays && displayCount > 0) ? displays[0] : 0;
                SDL_Rect usable{};
                if (display && SDL_GetDisplayUsableBounds(display, &usable)) {
                    const int tw = 460;
                    const bool sizingChunking =
                        toastProcessing && activeProgress && activeProgress->chunking.load();
                    const bool sizingAcknowledgement =
