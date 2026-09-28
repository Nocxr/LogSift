#pragma once

#include <imgui.h>
#include <string>

void ApplyLogSiftStyle();
ImVec2 ToastSourceBadgeSize(const std::string& sourceKind);
void DrawToastSourceBadge(const std::string& sourceKind, const ImVec4& color);
bool ToastSoundIconButton(bool enabled, const ImVec4& color, float size = 26.0f);
bool ToastCloseIconButton(const ImVec4& color, float size = 26.0f);
bool ToastGearIconButton(const ImVec4& color, float size = 26.0f);
bool ColoredCheckbox(const char* label, bool* value, const ImVec4& color);
void DrawStatusPill(const char* text, const ImVec4& color);
