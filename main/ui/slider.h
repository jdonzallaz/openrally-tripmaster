#pragma once

#include <M5GFX.h>
#include <M5Unified.h>
#include <stdint.h>

#include "../utils.h"
#include "swipe_manager.h"

/** Slider UI component with change handler. */
class Slider {
    void (*changeHandler)(int16_t);

    int16_t minValue{0};
    int16_t maxValue{100};
    int16_t currentValue{0};  // The actual value
    int16_t controlValue{0};  // The value while dragging

    int16_t x, y, w, h;

    /** Compute the position of the thumb based on the value. */
    int16_t computeThumbPosition(int16_t value, int16_t w, int16_t h) {
        return ((w) * (value - minValue)) / (maxValue - minValue);
    }

   public:
    void setup(int16_t minValue, int16_t maxValue, int16_t value) {
        assert(minValue < maxValue && minValue <= value && value <= maxValue);
        this->minValue = minValue;
        this->maxValue = maxValue;
        this->currentValue = value;
        this->controlValue = value;
    }

    void draw(LovyanGFX* gfx, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t borderColor,
              uint16_t backgroundColor, uint16_t thumbColor) {
        this->x = x;
        this->y = y;
        this->w = w;
        this->h = h;

        // Draw background and border
        gfx->drawRoundRect(x, y, w, h, 3, borderColor);
        gfx->fillRoundRect(x + 1, y + 1, w - 2, h - 2, 2, backgroundColor);

        // Draw thumb
        int16_t thumbPosition = computeThumbPosition(controlValue, w, h);
        gfx->fillSmoothCircle(x + thumbPosition, y + h / 2, h, thumbColor);

        // Draw background (left) of value
        gfx->fillRoundRect(x + 1, y + 1, thumbPosition, h - 2, 2, thumbColor);
    }

    void update(SwipeManager* swipe) {
        auto touch = M5.Touch.getDetail();  // TODO: pass by parameter - m5::touch_detail_t& td

        // Ignore touches outside the slider area
        if (!isContained(x, y, w, h, touch.base.x, touch.base.y, 10)) {
            return;
        }

        if (touch.wasPressed()) {
            swipe->invalidate();
        }

        // On release, update the value and call the change handler
        if (touch.wasReleased()) {
            currentValue = controlValue;
            changeHandler(currentValue);
        }

        // Move the thumb while dragging
        if (touch.isPressed()) {
            int16_t newValue = minValue + (maxValue - minValue) * (touch.x - x) / w;
            newValue = newValue < minValue ? minValue : newValue > maxValue ? maxValue : newValue;
            controlValue = newValue;
        }
    }

    void setChangeHandler(void (*handler)(int16_t)) { changeHandler = handler; }
};
