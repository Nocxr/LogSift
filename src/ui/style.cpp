#include "style.h"

#include <cmath>
#include <string>

// Shared ImGui styling and compact UI primitives.
// Included by main.cpp; keep this module focused on this responsibility.

void ApplyLogSiftStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    style.ScrollbarRounding = 4.0f;
    style.FramePadding = ImVec2(7.0f, 4.0f);
    style.ItemSpacing = ImVec2(7.0f, 5.0f);

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg]          = ImVec4(0.040f, 0.048f, 0.058f, 1.0f);
    c[ImGuiCol_ChildBg]           = ImVec4(0.050f, 0.064f, 0.078f, 1.0f);
    c[ImGuiCol_PopupBg]           = ImVec4(0.055f, 0.067f, 0.080f, 0.98f);
    c[ImGuiCol_Border]            = ImVec4(0.18f, 0.27f, 0.34f, 0.70f);
    c[ImGuiCol_FrameBg]           = ImVec4(0.075f, 0.12f, 0.17f, 1.0f);
    c[ImGuiCol_FrameBgHovered]    = ImVec4(0.10f, 0.20f, 0.29f, 1.0f);
    c[ImGuiCol_FrameBgActive]     = ImVec4(0.12f, 0.26f, 0.38f, 1.0f);
    c[ImGuiCol_TitleBg]           = ImVec4(0.045f, 0.060f, 0.073f, 1.0f);
    c[ImGuiCol_TitleBgActive]     = ImVec4(0.07f, 0.14f, 0.20f, 1.0f);
    c[ImGuiCol_Header]            = ImVec4(0.08f, 0.22f, 0.33f, 0.85f);
    c[ImGuiCol_HeaderHovered]     = ImVec4(0.10f, 0.32f, 0.46f, 0.92f);
    c[ImGuiCol_HeaderActive]      = ImVec4(0.10f, 0.38f, 0.54f, 1.0f);
    c[ImGuiCol_Button]            = ImVec4(0.08f, 0.25f, 0.38f, 1.0f);
    c[ImGuiCol_ButtonHovered]     = ImVec4(0.10f, 0.36f, 0.52f, 1.0f);
    c[ImGuiCol_ButtonActive]      = ImVec4(0.12f, 0.43f, 0.61f, 1.0f);
    c[ImGuiCol_CheckMark]         = ImVec4(0.20f, 0.82f, 1.00f, 1.0f);
    c[ImGuiCol_SliderGrab]        = ImVec4(0.23f, 0.69f, 0.92f, 1.0f);
    c[ImGuiCol_SliderGrabActive]  = ImVec4(0.28f, 0.86f, 1.00f, 1.0f);
    c[ImGuiCol_Tab]               = ImVec4(0.065f, 0.13f, 0.19f, 1.0f);
    c[ImGuiCol_TabHovered]        = ImVec4(0.10f, 0.32f, 0.46f, 1.0f);
    c[ImGuiCol_TabSelected]       = ImVec4(0.08f, 0.25f, 0.37f, 1.0f);
    c[ImGuiCol_Separator]         = ImVec4(0.20f, 0.34f, 0.43f, 0.70f);
    c[ImGuiCol_ResizeGrip]        = ImVec4(0.14f, 0.50f, 0.68f, 0.35f);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(0.18f, 0.67f, 0.90f, 0.70f);
    c[ImGuiCol_TextSelectedBg]    = ImVec4(0.10f, 0.38f, 0.56f, 0.60f);
}

ImVec2 ToastSourceBadgeSize(const std::string& sourceKind) {
    const char* label = sourceKind.empty() ? "Manual" : sourceKind.c_str();
    const float icon = 18.0f;
    const float badgeHeight = 24.0f;
    const float textWidth = ImGui::CalcTextSize(label).x;
    return ImVec2(icon + 8.0f + textWidth + 12.0f, badgeHeight);
}

void DrawToastSourceBadge(const std::string& sourceKind, const ImVec4& color) {
    const char* label = sourceKind.empty() ? "Manual" : sourceKind.c_str();
    const float icon = 18.0f;
    const float badgeHeight = 24.0f;
    const ImVec2 size = ToastSourceBadgeSize(sourceKind);
    const ImVec2 p = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton("##toast_source_badge", size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImU32 fg = ImGui::ColorConvertFloat4ToU32(color);
    const ImU32 bg = ImGui::ColorConvertFloat4ToU32(
        ImVec4(color.x, color.y, color.z, 0.12f));
    draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), bg, 5.0f);

    const ImVec2 q(p.x + 6.0f, p.y + 4.0f);
    if (sourceKind == "OCR") {
        // Four scan corners plus a small center lens.
        draw->AddLine({q.x, q.y + 5}, {q.x, q.y}, fg, 1.5f);
        draw->AddLine({q.x, q.y}, {q.x + 5, q.y}, fg, 1.5f);
        draw->AddLine({q.x + 12, q.y + 5}, {q.x + 12, q.y}, fg, 1.5f);
        draw->AddLine({q.x + 12, q.y}, {q.x + 7, q.y}, fg, 1.5f);
        draw->AddLine({q.x, q.y + 9}, {q.x, q.y + 14}, fg, 1.5f);
        draw->AddLine({q.x, q.y + 14}, {q.x + 5, q.y + 14}, fg, 1.5f);
        draw->AddLine({q.x + 12, q.y + 9}, {q.x + 12, q.y + 14}, fg, 1.5f);
        draw->AddLine({q.x + 12, q.y + 14}, {q.x + 7, q.y + 14}, fg, 1.5f);
        draw->AddCircle({q.x + 6, q.y + 7}, 2.2f, fg, 0, 1.4f);
    } else if (sourceKind == "File") {
        draw->AddRect({q.x + 1, q.y}, {q.x + 11, q.y + 14}, fg, 1.0f, 0, 1.4f);
        draw->AddLine({q.x + 7, q.y}, {q.x + 11, q.y + 4}, fg, 1.4f);
        draw->AddLine({q.x + 7, q.y}, {q.x + 7, q.y + 4}, fg, 1.4f);
        draw->AddLine({q.x + 7, q.y + 4}, {q.x + 11, q.y + 4}, fg, 1.4f);
    } else if (sourceKind == "Clipboard") {
        draw->AddRect({q.x + 1, q.y + 2}, {q.x + 12, q.y + 14}, fg, 2.0f, 0, 1.4f);
        draw->AddRectFilled({q.x + 4, q.y}, {q.x + 9, q.y + 4}, fg, 1.5f);
        draw->AddLine({q.x + 4, q.y + 7}, {q.x + 9, q.y + 7}, fg, 1.2f);
        draw->AddLine({q.x + 4, q.y + 10}, {q.x + 9, q.y + 10}, fg, 1.2f);
    } else {
        draw->AddLine({q.x + 1, q.y + 4}, {q.x + 11, q.y + 4}, fg, 1.4f);
        draw->AddLine({q.x + 1, q.y + 8}, {q.x + 11, q.y + 8}, fg, 1.4f);
        draw->AddLine({q.x + 1, q.y + 12}, {q.x + 8, q.y + 12}, fg, 1.4f);
    }

    const float textY = p.y + (badgeHeight - ImGui::GetTextLineHeight()) * 0.5f;
    draw->AddText({p.x + icon + 9.0f, textY}, fg, label);
}

bool ToastSoundIconButton(bool enabled, const ImVec4& color, float size) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton("##toast_sound_toggle", {size, size});
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    if (hovered) {
        draw->AddRectFilled(p, {p.x + size, p.y + size},
            IM_COL32(65, 75, 84, 150), 4.0f);
        ImGui::SetTooltip(enabled ? "Mute notification sounds" : "Unmute notification sounds");
    }

    // All three popup controls use the same centered 16x16 visual envelope.
    const ImU32 fg = ImGui::ColorConvertFloat4ToU32(color);
    const ImVec2 center{p.x + size * 0.5f, p.y + size * 0.5f};
    constexpr float half = 8.0f;
    const float left = center.x - half;
    const float speakerFront = center.x - 1.5f;
    ImVec2 speaker[6] = {
        {left, center.y - 3.5f}, {left + 4.5f, center.y - 3.5f},
        {speakerFront, center.y - 7.0f}, {speakerFront, center.y + 7.0f},
        {left + 4.5f, center.y + 3.5f}, {left, center.y + 3.5f}
    };
    draw->AddConvexPolyFilled(speaker, 6, fg);

    if (enabled) {
        draw->PathArcTo({speakerFront, center.y}, 5.0f, -0.72f, 0.72f, 10);
        draw->PathStroke(fg, 0, 1.5f);
        draw->PathArcTo({speakerFront, center.y}, 8.0f, -0.62f, 0.62f, 10);
        draw->PathStroke(fg, 0, 1.5f);
    } else {
        draw->AddLine(
            {center.x + 1.5f, center.y - 6.5f},
            {center.x + 7.5f, center.y + 6.5f}, fg, 1.8f);
        draw->AddLine(
            {center.x + 7.5f, center.y - 6.5f},
            {center.x + 1.5f, center.y + 6.5f}, fg, 1.8f);
    }
    return clicked;
}

bool ToastCloseIconButton(const ImVec4& color, float size) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton("##toast_close", {size, size});
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (hovered)
        draw->AddRectFilled(p, {p.x + size, p.y + size},
            IM_COL32(105, 42, 42, 180), 4.0f);
    const ImU32 fg = ImGui::ColorConvertFloat4ToU32(color);
    const ImVec2 center{p.x + size * 0.5f, p.y + size * 0.5f};
    constexpr float half = 7.0f;
    draw->AddLine(
        {center.x - half, center.y - half},
        {center.x + half, center.y + half}, fg, 1.9f);
    draw->AddLine(
        {center.x + half, center.y - half},
        {center.x - half, center.y + half}, fg, 1.9f);
    if (hovered) ImGui::SetTooltip("Close notification");
    return clicked;
}

bool ToastGearIconButton(const ImVec4& color, float size) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton("##toast_gear", {size, size});
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (hovered) {
        draw->AddRectFilled(p, {p.x + size, p.y + size},
            IM_COL32(58, 72, 86, 180), 4.0f);
        ImGui::SetTooltip("Open Log Sift");
    }

    const ImU32 fg = ImGui::ColorConvertFloat4ToU32(color);
    const ImVec2 center{p.x + size * 0.5f, p.y + size * 0.5f};
    constexpr float ring = 5.2f;
    constexpr float toothInner = 6.3f;
    constexpr float toothOuter = 8.0f;
    draw->AddCircle(center, ring, fg, 16, 1.7f);
    draw->AddCircle(center, 2.2f, fg, 12, 1.7f);
    for (int i = 0; i < 8; ++i) {
        const float a = static_cast<float>(i) * 3.14159265f / 4.0f;
        const ImVec2 a0{
            center.x + std::cos(a) * toothInner,
            center.y + std::sin(a) * toothInner};
        const ImVec2 a1{
            center.x + std::cos(a) * toothOuter,
            center.y + std::sin(a) * toothOuter};
        draw->AddLine(a0, a1, fg, 2.0f);
    }
    return clicked;
}

bool ColoredCheckbox(const char* label, bool* value, const ImVec4& color) {
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    const bool changed = ImGui::Checkbox(label, value);
    ImGui::PopStyleColor();
    return changed;
}

void DrawStatusPill(const char* text, const ImVec4& color) {
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 size{textSize.x + 14.0f, textSize.y + 6.0f};
    ImGui::InvisibleButton(text, size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(
        p, {p.x + size.x, p.y + size.y},
        ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, 0.15f)),
        5.0f);
    draw->AddRect(
        p, {p.x + size.x, p.y + size.y},
        ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, 0.45f)),
        5.0f, 0, 1.0f);
    draw->AddText({p.x + 7.0f, p.y + 3.0f},
        ImGui::ColorConvertFloat4ToU32(color), text);
}

