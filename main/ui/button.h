#pragma once

#include <M5GFX.h>
#include <M5Unified.h>
#include <stdint.h>

/** Button UI component with click and hold handlers. */
class Button {
    enum class State {
        IDLE,
        PRESSED,
        HOLD,
    };

    LGFX_Button button;
    State state = State::IDLE;
    void (*clickHandler)();
    void (*holdHandler)();

   public:
    template <typename T>
    void draw(LovyanGFX* gfx, int16_t x, int16_t y, uint16_t w, uint16_t h, const T& outline, const T& fill,
              const T& textcolor, const char* label, float textsize_x = 1.0f, float textsize_y = 0.0f) {
        button.initButtonUL(gfx, x, y, w, h, outline, fill, textcolor, label, textsize_x, textsize_y);  // TODO: move to init
        button.drawButton(state != State::IDLE);
    }

    void update() {
        auto touch = M5.Touch.getDetail();

        // Set inner pressed state of button
        if (touch.isPressed() && button.contains(touch.x, touch.y))
            button.press(true);
        else if (touch.wasReleased() || !button.contains(touch.x, touch.y)) {
            button.press(false);
        }

        // Set hold state of button
        if (touch.wasHold() && button.isPressed()) {
            state = State::HOLD;
        }

        // Set pressed button state according to inner state
        if (touch.wasPressed() && button.justPressed()) {
            state = State::PRESSED;
        }

        // Set released button state according to inner state
        if (button.justReleased()) {
            if (touch.wasReleased()) {
                if (state == State::PRESSED) {
                    if (clickHandler) {
                        clickHandler();
                    }

                } else if (state == State::HOLD) {
                    if (holdHandler) {
                        holdHandler();
                    }
                }
            }

            state = State::IDLE;
        }
    }

    void setClickHandler(void (*handler)()) { clickHandler = handler; }

    void setHoldHandler(void (*handler)()) { holdHandler = handler; }
};
