// Async joins and SDL/ImGui/platform shutdown.
// Included by main.cpp; keep this module focused on this responsibility.

    if (busy) request.wait();
    if (loadingModels) modelListRequest.wait();
    if (applyingCompute) computeRequest.wait();
    if (checkingHealth) healthRequest.wait();
    if (benchmarking) benchmarkRequest.wait();
#ifdef _WIN32
    if (gTrayHwnd) {
        Shell_NotifyIconW(NIM_DELETE, &gTrayIcon);
        DestroyWindow(gTrayHwnd);
    }
    if (gAppIconSmall) { DestroyIcon(gAppIconSmall); gAppIconSmall = nullptr; }
    if (gAppIconBig) { DestroyIcon(gAppIconBig); gAppIconBig = nullptr; }
#endif
    if (toastContext) {
        ImGui::SetCurrentContext(toastContext);
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(toastContext);
        if (toastRenderer) SDL_DestroyRenderer(toastRenderer);
        if (toastWindow) SDL_DestroyWindow(toastWindow);
    }
    ImGui::SetCurrentContext(mainContext);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(mainContext);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    ShutdownNotificationAudio();
    SDL_Quit();
#ifdef _WIN32
    if (gSingleInstanceMutex) {
        CloseHandle(gSingleInstanceMutex);
        gSingleInstanceMutex = nullptr;
    }
#endif
    return 0;
}
