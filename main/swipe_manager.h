#pragma once

#include <M5Unified.h>

#include "constants.h"

enum class SwipeDirection {
    UP,
    DOWN,
    RIGHT,
    LEFT,
};

/** Manages swipe gestures on the screen with callbacks. */
class SwipeManager {
    void (*swipeHandler)(SwipeDirection);
    bool invalidated{false};

   public:
    /** Update the swipe manager, checking for swipe gestures and calling the swipe handler. */
    void update() {
        // Ignore processing if no touch detected or no handler set
        if (M5.Touch.getCount() == 0 || !swipeHandler) {
            return;
        }

        auto touch = M5.Touch.getDetail();  // TODO: pass by parameter

        // Check for swipe gestures and call the handler
        if (touch.wasFlicked() && !invalidated) {
            if (abs(touch.distanceX()) > SWIPE_HORIZONTAL_THRESHOLD &&
                abs(touch.distanceY()) < SWIPE_MAX_OTHER_DIRECTION_THRESHOLD) {
                swipeHandler(touch.distanceX() > 0 ? SwipeDirection::RIGHT : SwipeDirection::LEFT);
            } else if (abs(touch.distanceY()) > SWIPE_VERTICAL_THRESHOLD &&
                       abs(touch.distanceX()) < SWIPE_MAX_OTHER_DIRECTION_THRESHOLD) {
                swipeHandler(touch.distanceY() > 0 ? SwipeDirection::DOWN : SwipeDirection::UP);
            }
        }

        if (touch.wasReleased()) {
            invalidated = false;
        }
    }

    /** Invalidate the current swipe, preventing further swipe events until the next touch release. */
    void invalidate() { invalidated = true; }

    /** Set the swipe handler callback. */
    void setSwipeHandler(void (*handler)(SwipeDirection)) { swipeHandler = handler; }
};
