#pragma once

#include <chrono>

struct PopupTimerState {
    bool paused = false;
    bool mouseWasOver = false;
    bool resumeRequested = false;
    std::chrono::steady_clock::duration pausedRemaining{};
    std::chrono::steady_clock::time_point mouseLeftAt{};
    std::chrono::steady_clock::time_point lastShownAt{};
};

// Updates the hover pause and expiration for one notification.
bool AdvancePopupTimer(PopupTimerState& timer,
                       std::chrono::steady_clock::time_point now,
                       std::chrono::steady_clock::time_point shownAt,
                       std::chrono::steady_clock::time_point& until,
                       bool processing, bool hasText,
                       bool hoverEnabled, bool mouseOver);
