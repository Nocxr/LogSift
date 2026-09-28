// Activity window, main render, and config persistence.
// Included by main.cpp; keep this module focused on this responsibility.

        ImGui::End();

        if (showAppLog) {
            ImGui::SetNextWindowSize({760, 300}, ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Log Sift Output", &showAppLog)) {
                ImGui::TextDisabled("Live activity: health, models, LLM requests, chunking, copies, fallbacks, success/failure - grave key toggles");
                if (ImGui::Button("Clear Log")) appLog.clear();
                ImGui::Separator();
                ImGui::InputTextMultiline("##applog", &appLog, {-1, -1}, ImGuiInputTextFlags_ReadOnly);
            }
            ImGui::End();
        }

        const bool mainVisibleForRender = (SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) == 0;
        if (mainVisibleForRender) {
            ImGui::Render();
            SDL_SetRenderDrawColor(renderer, 18,18,20,255);
            SDL_RenderClear(renderer);
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
            SDL_RenderPresent(renderer);
        } else {
            ImGui::EndFrame();
        }

        const std::string configSnapshot = ConfigToJson(cfg).dump();
        if (configSnapshot != lastSavedConfig) {
            SaveConfig(cfg);
            lastSavedConfig = configSnapshot;
        }
        }
