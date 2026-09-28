// Frame pacing for visible, popup, and background states.
// Included by main.cpp; keep this module focused on this responsibility.

        const bool asyncActive = busy || checkingHealth || benchmarking || loadingModels || applyingCompute;
        const bool mainVisible = (SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) == 0;

        if (mainVisible) {
            SDL_Delay(16u); // visible main window: fixed ~60 FPS
        } else if (toastActive) {
            const Uint32 frameMs = static_cast<Uint32>(std::max(1, 1000 / std::max(1, cfg.toastFps)));
            SDL_Delay(frameMs);
        } else if (asyncActive) {
            SDL_Delay(75u);
        } else {
            SDL_Delay(100u);
        }
    }
