#pragma once

#include <M5GFX.h>
#include <M5Unified.h>
#include <stdint.h>

#include "../utils.h"

/** Switch/toggle UI component with change handler. */
class Switch {
    void (*changeHandler)(bool);

    bool value{false};

    int16_t x, y, w, h;

   public:
    void setup(bool value) { this->value = value; }

    void draw(LovyanGFX* gfx, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t borderColor, uint16_t enabledColor,
              uint16_t disabledColor, uint16_t thumbColor = WHITE) {
        this->x = x;
        this->y = y;
        this->w = w;
        this->h = h;

        // Draw background and border
        uint16_t backgroundColor = value ? enabledColor : disabledColor;
        gfx->fillRoundRect(x, y, w, h, h / 2, backgroundColor);

        // Draw thumb
        int16_t thumbPosition = value ? w - h / 2 : h / 2;
        gfx->fillSmoothCircle(x + thumbPosition, y + h / 2, h / 2 - 3, thumbColor);
    }

    void update() {
        auto touch = M5.Touch.getDetail();  // TODO: pass by parameter

        // Ignore touches outside the switch area
        if (!isContained(x, y, w, h, touch.base.x, touch.base.y, 5)) {
            return;
        }

        // On release, update the value and call the change handler
        if (touch.wasReleased()) {
            value = !value;
            changeHandler(value);
        }
    }

    void setChangeHandler(void (*handler)(bool)) { changeHandler = handler; }
};
