#pragma once

#include <M5GFX.h>
#include <stdint.h>

#include <string>

#include "../constants.h"

M5Canvas alertSprite;

/** Draw an alert message centered on the screen. */
void drawAlert(LovyanGFX* destination, const std::string& message, uint8_t nLine = 1) {
    uint16_t height = UI_ALERT_HEIGHT_LINE_HEIGHT * nLine;
    alertSprite.createSprite(UI_ALERT_WIDTH, height);
    alertSprite.fillSprite(BLACK);

    alertSprite.fillRoundRect(0, 0, UI_ALERT_WIDTH, height, 8, COLOR_LIGHTGREY);
    alertSprite.drawRoundRect(0, 0, UI_ALERT_WIDTH, height, 8, COLOR_ERROR);

    alertSprite.setTextColor(COLOR_ERROR);
    alertSprite.setFont(&FreeSans12pt7b);
    alertSprite.setTextDatum(top_center);
    alertSprite.drawString(message.c_str(), UI_ALERT_WIDTH / 2, 10);

    alertSprite.pushSprite(destination, (DISPLAY_WIDTH - UI_ALERT_WIDTH) / 2, (DISPLAY_HEIGHT - height) / 2, BLACK);
}
