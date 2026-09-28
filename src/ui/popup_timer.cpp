#include "popup_timer.h"

#include <algorithm>

bool AdvancePopupTimer(PopupTimerState& timer,
                       std::chrono::steady_clock::time_point now,
                       std::chrono::steady_clock::time_point shownAt,
                       std::chrono::steady_clock::time_point& until,
                       bool processing, bool hasText,
                       bool hoverEnabled, bool mouseOver) {
    if (shownAt != timer.lastShownAt) {
        timer.lastShownAt = shownAt;
        timer.paused = false;
        timer.mouseWasOver = false;
        timer.resumeRequested = false;
        timer.pausedRemaining = {};
        timer.mouseLeftAt = {};
    }

    // Dismissing a hovered popup must release its paused state. Otherwise
    // the tray loop keeps treating an invisible popup as active.
    if (!hasText) {
        timer.paused = false;
        timer.mouseWasOver = false;
        timer.resumeRequested = false;
        timer.pausedRemaining = {};
        timer.mouseLeftAt = {};
        return false;
    }

    if (!processing && hasText && hoverEnabled) {
        if (mouseOver) {
            if (!timer.paused) {
                timer.pausedRemaining = std::max(
                    std::chrono::steady_clock::duration::zero(), until - now);
                timer.paused = true;
            }
            timer.mouseLeftAt = {};
            timer.mouseWasOver = true;
        } else if (timer.paused) {
            if (timer.resumeRequested) {
                until = now + timer.pausedRemaining;
                timer.paused = false;
                timer.mouseLeftAt = {};
                timer.resumeRequested = false;
            } else {
                if (timer.mouseWasOver && timer.mouseLeftAt.time_since_epoch().count() == 0)
                    timer.mouseLeftAt = now;
                constexpr auto kHoverGrace = std::chrono::seconds(2);
                if (timer.mouseLeftAt.time_since_epoch().count() != 0 &&
                    now - timer.mouseLeftAt >= kHoverGrace) {
                    until = now + timer.pausedRemaining;
                    timer.paused = false;
                    timer.mouseLeftAt = {};
                }
            }
            timer.mouseWasOver = false;
        }
    }
    return hasText && (processing || timer.paused || now < until);
}
