#pragma once

#include <M5GFX.h>
#include <M5Unified.h>
#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "../utils.h"
#include "fonts/FreeSans7pt7b.h"

/** Radio button option. The value will be passed to the change handler. */
struct RadioButtonOption {
    uint8_t value;
    std::string label;
};

/** Radio button UI component with change handler. */
class RadioButton {
    M5Canvas sprite;
    void (*changeHandler)(uint8_t);
    std::vector<RadioButtonOption> options;
    uint8_t value{0};
    int16_t x, y, w, h;

   public:
    void setup(const std::vector<RadioButtonOption>& options, uint8_t value) {
        this->options = options;
        this->value = value;
    }

    void draw(LovyanGFX* gfx, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t border, uint16_t background,
              uint16_t highlight) {
        this->x = x;
        this->y = y;
        this->w = w;
        this->h = h;

        uint16_t optionWidth = w / options.size();

        sprite.createSprite(w, h);
        sprite.fillSprite(WHITE);
        sprite.setFont(&FreeSans7pt7b);
        sprite.setTextDatum(middle_center);

        for (size_t i = 0; i < options.size(); i++) {
            // Draw the outline
            sprite.drawRect(i * optionWidth, 0, optionWidth, h, border);
            // Draw the background of the (selected) option
            sprite.fillRect(1 + i * optionWidth, 1, optionWidth - 2, h - 2,
                            value == options[i].value ? highlight : background);
            // Draw the text
            sprite.setTextColor(value == options[i].value ? background : highlight);
            sprite.drawString(options[i].label.c_str(), optionWidth / 2 + i * optionWidth, h / 2 + 1);
        }

        sprite.pushSprite(gfx, x, y);
    }

    void update() {
        auto touch = M5.Touch.getDetail();  // TODO: pass by parameter (remove include)

        // Ignore touches outside the button area
        if (!isContained(x, y, w, h, touch.base.x, touch.base.y, 10)) {
            return;
        }

        // React to touch events, set new value and call change handler
        if (touch.wasClicked()) {
            uint8_t newValue = (touch.x - x) / (w / options.size());
            if (newValue != value) {
                value = newValue;
                if (changeHandler) {
                    changeHandler(value);
                }
            }
        }
    }

    void setValue(uint8_t newValue) { value = newValue; }

    void setChangeHandler(void (*handler)(uint8_t)) { changeHandler = handler; }
};
