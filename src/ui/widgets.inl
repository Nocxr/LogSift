// Reusable popup disclosure/copy-preview widgets.
// Included by main.cpp; keep this module focused on this responsibility.

bool ToastDisclosureRow(const char* id, const char* label, bool& expanded, const ImVec4& accent) {
    ImGui::PushID(id);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(accent.x, accent.y, accent.z, 0.10f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(accent.x, accent.y, accent.z, 0.18f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(accent.x, accent.y, accent.z, 0.24f));

    std::string text = expanded ? "[-] " : "[+] ";
    text += label;
    if (ImGui::Selectable(
            text.c_str(), false, ImGuiSelectableFlags_None,
            ImVec2(ImGui::GetContentRegionAvail().x, 22.0f)))
        expanded = !expanded;

    ImGui::PopStyleColor(3);
    ImGui::PopID();
    return expanded;
}

struct CopyFlashState {
    std::chrono::steady_clock::time_point until{};
    size_t entryHash = 0;
    bool all = false;
};

void StartCopyFlash(CopyFlashState& flash, bool all, size_t entryHash = 0) {
    flash.until = std::chrono::steady_clock::now() + std::chrono::milliseconds(420);
    flash.all = all;
    flash.entryHash = entryHash;
}

void DrawDiagnosticEntries(const char* id, const std::string& text, float height,
    std::string& status, std::string& lastClipboardText, std::string& appLog,
    CopyFlashState& copyFlash) {
    ImGui::BeginChild(id, {-1, height}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
    const auto entries = DiagnosticEntries(text);
    if (entries.empty()) ImGui::TextDisabled("No entries.");
    for (size_t i = 0; i < entries.size(); ++i) {
        const std::string& entry = entries[i];
        const bool error = entry.find("Error:") != std::string::npos || entry.find("error ") != std::string::npos ||
                           entry.find("Fatal") != std::string::npos || entry.find("fatal") != std::string::npos;
        const bool warning = entry.find("Warning:") != std::string::npos || entry.find("warning ") != std::string::npos;
        if (error) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
        else if (warning) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.82f, 0.25f, 1.0f));

        ImGui::PushID(static_cast<int>(i));
        ImGui::BeginGroup();
        const ImVec2 topLeft = ImGui::GetCursorScreenPos();
        ImGui::TextUnformatted(entry.c_str());
        ImGui::EndGroup();
        const ImVec2 bottomRight = ImGui::GetItemRectMax();

        const auto flashNow = std::chrono::steady_clock::now();
        const size_t entryHash = std::hash<std::string>{}(entry);
        if (flashNow < copyFlash.until && (copyFlash.all || copyFlash.entryHash == entryHash)) {
            const float remaining = std::chrono::duration<float>(copyFlash.until - flashNow).count();
            const float t = std::clamp(remaining / 0.42f, 0.0f, 1.0f);
            const float pulse = 0.10f + 0.16f * t;
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(
                ImVec2(topLeft.x - 4.0f, topLeft.y - 2.0f),
                ImVec2(bottomRight.x + 4.0f, bottomRight.y + 2.0f),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.18f, 0.82f, 1.0f, pulse)),
                4.0f);
            drawList->AddRect(
                ImVec2(topLeft.x - 4.0f, topLeft.y - 2.0f),
                ImVec2(bottomRight.x + 4.0f, bottomRight.y + 2.0f),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.30f, 0.92f, 1.0f, 0.35f * t)),
                4.0f, 0, 1.0f);
        }

        ImGui::SetCursorScreenPos(topLeft);
        ImGui::SetNextItemAllowOverlap();
        if (ImGui::InvisibleButton("##entry_click", {std::max(1.0f, bottomRight.x - topLeft.x), std::max(ImGui::GetTextLineHeightWithSpacing(), bottomRight.y - topLeft.y)})) {
            if (SetOwnedClipboardText(entry, &lastClipboardText)) {
                StartCopyFlash(copyFlash, false, entryHash);
                status = "Copied diagnostic entry to clipboard.";
                AppendActivityLog(appLog, "COPY", "Individual diagnostic copied.");
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::BeginTooltip();
            ImGui::TextUnformatted("Click to copy this entry");
            ImGui::EndTooltip();
        }
        ImGui::SetCursorScreenPos({topLeft.x, bottomRight.y + 3.0f});
        ImGui::PopID();
        if (error || warning) ImGui::PopStyleColor();
        ImGui::Separator();
    }
    ImGui::EndChild();
}

void DrawToastPreviewEntries(const char* id, const std::vector<std::string>& entries,
    size_t previewCount, float height,
    std::string& status, std::string& lastClipboardText, std::string& appLog,
    CopyFlashState& copyFlash) {

    ImGui::BeginChild(id, {-1, height}, ImGuiChildFlags_Borders,
        ImGuiWindowFlags_None);
    if (previewCount == 0) {
        ImGui::TextDisabled("No entries.");
        ImGui::EndChild();
        return;
    }

    for (size_t i = 0; i < previewCount; ++i) {
        const std::string& entry = entries[i];
        const bool error =
            entry.find("Error:") != std::string::npos ||
            entry.find("error ") != std::string::npos ||
            entry.find("Fatal") != std::string::npos ||
            entry.find("fatal") != std::string::npos;
        const bool warning =
            entry.find("Warning:") != std::string::npos ||
            entry.find("warning ") != std::string::npos;

        if (error) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
        else if (warning) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.82f, 0.25f, 1.0f));

        ImGui::PushID(static_cast<int>(i));
        const ImVec2 topLeft = ImGui::GetCursorScreenPos();
        const float available = std::max(80.0f, ImGui::GetContentRegionAvail().x - 8.0f);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + available);
        ImGui::TextWrapped("%s", entry.c_str());
        ImGui::PopTextWrapPos();
        const ImVec2 textMax = ImGui::GetItemRectMax();
        const float rowHeight = std::max(
            ImGui::GetTextLineHeightWithSpacing(),
            textMax.y - topLeft.y);

        const auto flashNow = std::chrono::steady_clock::now();
        const size_t entryHash = std::hash<std::string>{}(entry);
        if (flashNow < copyFlash.until &&
            (copyFlash.all || copyFlash.entryHash == entryHash)) {
            const float remaining =
                std::chrono::duration<float>(copyFlash.until - flashNow).count();
            const float t = std::clamp(remaining / 0.42f, 0.0f, 1.0f);
            ImGui::GetWindowDrawList()->AddRectFilled(
                {topLeft.x - 3.0f, topLeft.y - 2.0f},
                {topLeft.x + available + 3.0f, topLeft.y + rowHeight + 2.0f},
                ImGui::ColorConvertFloat4ToU32(
                    ImVec4(0.18f, 0.82f, 1.0f, 0.10f + 0.16f * t)),
                4.0f);
        }

        ImGui::SetCursorScreenPos(topLeft);
        ImGui::SetNextItemAllowOverlap();
        if (ImGui::InvisibleButton(
                "##preview_entry_click",
                {available, rowHeight})) {
            if (SetOwnedClipboardText(entry, &lastClipboardText)) {
                StartCopyFlash(copyFlash, false, entryHash);
                status = "Copied diagnostic entry to clipboard.";
                AppendActivityLog(appLog, "COPY",
                    "Popup preview diagnostic copied.");
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("Click to copy this entry");
        }

        ImGui::SetCursorScreenPos({topLeft.x, topLeft.y + rowHeight + 4.0f});
        if (i + 1 < previewCount) ImGui::Separator();
        ImGui::PopID();
        if (error || warning) ImGui::PopStyleColor();
    }
    ImGui::EndChild();
}

