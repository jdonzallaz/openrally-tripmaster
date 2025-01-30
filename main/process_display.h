#pragma once

#include <M5GFX.h>
#include <M5Unified.h>
#include <math.h>
#include <stdint.h>

#include <format>
#include <string>
#include <vector>

#include "constants.h"
#include "fonts/FreeSans40pt7b.h"
#include "fonts/FreeSans48pt7b.h"
#include "fonts/FreeSans56pt7b.h"
#include "fonts/icons.h"
#include "freertos/FreeRTOS.h"
#include "state.h"
#include "swipe_manager.h"
#include "ui/button.h"
#include "ui/radio_button.h"
#include "ui/slider.h"
#include "ui/switch.h"
#include "utils.h"

// UI States
enum StateUiScreen {
    MAIN,
    PARAMETERS,
};
enum StateUiMainScreen : uint8_t {
    MAIN_PAGE_1,
    MAIN_PAGE_2,
};
enum StateUiParameterScreen {
    PARAMETERS_PAGE_1,
    PARAMETERS_PAGE_2,
};

// Sizes for the distance display
enum UiDistanceSize {
    DISTANCE_SIZE_SMALL,
    DISTANCE_SIZE_LARGE,
};

// States for the FSM of the UI
StateUiScreen stateUiScreen{MAIN};
StateUiMainScreen stateUiMainScreen{MAIN_PAGE_1};
StateUiParameterScreen stateUiParametersScreen{PARAMETERS_PAGE_1};

// Main instances of the UI components
M5GFX display;
SwipeManager swipe;
M5Canvas mainSprite;

/** Handle swipe gestures and change the UI state accordingly. */
void swipeHandler(SwipeDirection direction) {
    if (stateUiScreen == MAIN && direction == SwipeDirection::UP) {
        stateUiScreen = PARAMETERS;
    } else if (stateUiScreen == PARAMETERS && direction == SwipeDirection::DOWN) {
        stateUiScreen = MAIN;
    } else if (stateUiScreen == MAIN && (direction == SwipeDirection::LEFT || direction == SwipeDirection::RIGHT)) {
        stateUiMainScreen = stateUiMainScreen == MAIN_PAGE_1 ? MAIN_PAGE_2 : MAIN_PAGE_1;
        sharedState.setPage(stateUiMainScreen);
    } else if (stateUiScreen == PARAMETERS &&
               (direction == SwipeDirection::RIGHT || direction == SwipeDirection::LEFT)) {
        stateUiParametersScreen = stateUiParametersScreen == PARAMETERS_PAGE_1 ? PARAMETERS_PAGE_2 : PARAMETERS_PAGE_1;
    }
}

/** Draw distance in small format */
void drawDistanceSmall(uint8_t distHundreds, uint8_t distTensUnits, uint8_t distDecimal) {
    uint8_t distInt = distHundreds * 100 + distTensUnits;

    // Draw distance: hundreds, tens and units + dot
    mainSprite.setFont(&FreeSans40pt7b);
    mainSprite.setTextDatum(top_right);
    mainSprite.drawString(formatString("%3d.", distInt).c_str(), 175, 43);

    // Draw distance: decimal
    mainSprite.setFont(&FreeSans48pt7b);
    mainSprite.drawString(formatString("%02d", distDecimal).c_str(), 280, 30);
}

/** Draw distance in big format */
void drawDistanceBig(uint8_t distHundreds, uint8_t distTensUnits, uint8_t distDecimal) {
    // Draw distance: hundreds
    if (distHundreds > 0) {
        mainSprite.setTextDatum(top_left);
        mainSprite.setFont(&FreeSans40pt7b);
        mainSprite.drawNumber(distHundreds, 9, 43);
    }

    // Draw distance: tens and units + dot
    mainSprite.setFont(&FreeSans48pt7b);
    mainSprite.setTextDatum(top_right);
    const char* distTensUnitsFormat = distHundreds > 0 ? "%02d." : "%2d.";
    mainSprite.drawString(formatString(distTensUnitsFormat, distTensUnits).c_str(), 183, 31);

    // Draw distance: decimal
    mainSprite.setFont(&FreeSans56pt7b);
    mainSprite.drawString(formatString("%02d", distDecimal).c_str(), 308, 20);
}

/** Draw the distance on the screen */
void drawDistance(UiDistanceSize size) {
    // Get and convert distance for printing
    float stageDistance = sharedState.getStageDistance() / 1000;                       // 123.4567
    uint8_t distInt = static_cast<uint8_t>(stageDistance);                             // 123
    uint8_t distHundreds = distInt / 100;                                              // 1
    uint8_t distTensUnits = distInt % 100;                                             // 23
    float distDecimal = stageDistance - distInt;                                       // 0.4567
    uint8_t distDecimal2places = static_cast<uint8_t>(std::round(distDecimal * 100));  // 45

    // Select the drawing function based on the size
    void (*drawDistanceFn)(uint8_t, uint8_t, uint8_t) =
        size == DISTANCE_SIZE_SMALL ? drawDistanceSmall : drawDistanceBig;
    drawDistanceFn(distHundreds, distTensUnits, distDecimal2places);
}

/** Draw the main screen with all the information */
void drawCompleteScreen(const M5Canvas& sprite) {
    drawDistance(DISTANCE_SIZE_SMALL);

    // Draw cap
    uint16_t cap = sharedState.getCap();
    mainSprite.setTextDatum(top_right);
    mainSprite.setFont(&FreeSans40pt7b);
    mainSprite.drawNumber(cap, 155, 155);
    mainSprite.setTextDatum(top_left);
    mainSprite.fillCircle(170, 165, 10);  // Custom degree (°) not supported by the font
    mainSprite.fillCircle(170, 165, 5, WHITE);

    // Draw speed
    mainSprite.setTextDatum(top_right);
    mainSprite.setFont(&FreeSans40pt7b);
    float speed = sharedState.getSpeed();
    mainSprite.drawNumber(speed, 310, 126);
    mainSprite.setFont(&DejaVu12);
    mainSprite.drawString("km/h", 306, 190);

    // Draw satellites
    u_int8_t satellites = sharedState.getNbSatellites();
    mainSprite.setTextDatum(top_right);
    mainSprite.setTextColor(DARKGREY);
    mainSprite.setFont(&FreeSans9pt7b);
    mainSprite.drawNumber(satellites, 298, 2);
    mainSprite.drawXBitmap(302, 2, icon::satellite, ICON_WIDTH, ICON_HEIGHT, DARKGREY);
    mainSprite.setTextDatum(top_left);

    // Draw time
    Time time = sharedState.getTime();
    mainSprite.setTextDatum(bottom_right);
    mainSprite.setTextColor(DARKGREY);
    mainSprite.setFont(&FreeSans9pt7b);
    mainSprite.drawString(formatString("%02d:%02d", time.hour, time.minute).c_str(), 317, 239);
    mainSprite.setTextDatum(top_left);
    mainSprite.setTextColor(BLACK);
}

/** Draw minimal screen with only the distance and cap */
void drawMinimalScreen(const M5Canvas& sprite) {
    drawDistance(DISTANCE_SIZE_LARGE);

    // Draw cap
    uint16_t cap = sharedState.getCap();
    mainSprite.setTextDatum(top_right);
    mainSprite.setFont(&FreeSans48pt7b);
    mainSprite.drawNumber(cap, 200, 140);
    mainSprite.setTextDatum(top_left);
    mainSprite.fillCircle(216, 155, 10);  // Custom degree (°) not supported by the font
    mainSprite.fillCircle(216, 155, 5, WHITE);
}

/** Draw the main screen */
void drawMainScreen() {
    display.beginTransaction();
    mainSprite.fillSprite(WHITE);

    // Draw small titles
    mainSprite.setFont(&DejaVu12);
    mainSprite.setTextColor(BLACK);
    mainSprite.drawString("DIST", 10, 6);
    mainSprite.drawString("CAP", 10, 126);

    // Draw custom info for each screen
    if (stateUiMainScreen == MAIN_PAGE_1) {
        drawCompleteScreen(mainSprite);
    } else if (stateUiMainScreen == MAIN_PAGE_2) {
        drawMinimalScreen(mainSprite);
    }

    // Errors and alerts
    if (sharedState.getNbSatellites() <= 0) {
        mainSprite.setFont(&FreeSans40pt7b);
        mainSprite.setTextColor(COLOR_ERROR);
        mainSprite.drawString("*", 155, 2);
    }

    mainSprite.pushSprite(&display, 0, 0);
    display.endTransaction();
}

// Parameters screen components
// Stage distance
Button buttonStageIncrease;
Button buttonStageDecrease;
Button buttonStageReset;
// Distance mode
RadioButton radioButtonDistanceMode;
// Wheel size
Button buttonWheelSizeIncrease;
Button buttonWheelSizeDecrease;
// Timezone
Button buttonTimezoneIncrease;
Button buttonTimezoneDecrease;
// Brightness
Slider sliderBrightness;

/** Draw the parameter screen */
void drawParametersScreen() {
    display.beginTransaction();
    mainSprite.fillSprite(WHITE);

    // Screen title
    mainSprite.setFont(&FreeSansBold9pt7b);
    mainSprite.setTextColor(BLACK);
    mainSprite.setTextDatum(top_left);
    mainSprite.drawString("Parameters", 10, 6);
    mainSprite.setFont(&FreeSans9pt7b);
    mainSprite.drawString("1/2", 290, 6);
    mainSprite.drawFastHLine(0, 27, 320, COLOR_LIGHTGREY);

    // Global parameters for all components
    mainSprite.setTextColor(BLACK);
    mainSprite.setTextDatum(middle_left);
    uint16_t xLabels = 10;
    uint16_t xValues = 108;
    constexpr uint16_t y[]{49, 91, 133, 175, 217};

    // Draw stage distance
    mainSprite.drawString("Stage", xLabels, y[0]);
    mainSprite.drawString(formatString("%.2f km", sharedState.getStageDistance() / 1000).c_str(), xValues - 30, y[0]);
    mainSprite.setFont(&FreeMonoBold9pt7b);
    buttonStageDecrease.draw(&mainSprite, 170, y[0] - 15, 30, 30, UI_BUTTON_BORDER_COLOR, UI_BUTTON_BACKGROUND_COLOR,
                             UI_BUTTON_TEXT_COLOR, "-");
    buttonStageIncrease.draw(&mainSprite, 210, y[0] - 15, 30, 30, UI_BUTTON_BORDER_COLOR, UI_BUTTON_BACKGROUND_COLOR,
                             UI_BUTTON_TEXT_COLOR, "+");
    mainSprite.setFont(&FreeSans9pt7b);
    buttonStageReset.draw(&mainSprite, 250, y[0] - 15, 60, 30, UI_BUTTON_BORDER_COLOR, UI_BUTTON_BACKGROUND_COLOR,
                          UI_BUTTON_TEXT_COLOR, "Reset");

    // Draw distance mode
    mainSprite.drawString("Mode", xLabels, y[1]);
    radioButtonDistanceMode.draw(&mainSprite, xValues, y[1] - 15, 202, 30, COLOR_PRIMARY, WHITE, COLOR_PRIMARY);

    // Draw wheel size
    mainSprite.drawString("Wheel size", xLabels, y[2]);
    mainSprite.drawString(formatString("%d mm", sharedState.getWheelSize()).c_str(), xValues, y[2]);
    mainSprite.setFont(&FreeMonoBold9pt7b);
    buttonWheelSizeDecrease.draw(&mainSprite, 240, y[2] - 15, 30, 30, UI_BUTTON_BORDER_COLOR,
                                 UI_BUTTON_BACKGROUND_COLOR, UI_BUTTON_TEXT_COLOR, "-");
    buttonWheelSizeIncrease.draw(&mainSprite, 280, y[2] - 15, 30, 30, UI_BUTTON_BORDER_COLOR,
                                 UI_BUTTON_BACKGROUND_COLOR, UI_BUTTON_TEXT_COLOR, "+");
    mainSprite.setFont(&FreeSans9pt7b);

    // Draw timezone
    mainSprite.drawString("Timezone", xLabels, y[3]);
    mainSprite.drawString(formatString("%+03d:00", sharedState.getTimezone()).c_str(), xValues - 10, y[3]);
    mainSprite.setFont(&FreeMonoBold9pt7b);
    buttonTimezoneDecrease.draw(&mainSprite, 240, y[3] - 15, 30, 30, UI_BUTTON_BORDER_COLOR, UI_BUTTON_BACKGROUND_COLOR,
                                UI_BUTTON_TEXT_COLOR, "-");
    buttonTimezoneIncrease.draw(&mainSprite, 280, y[3] - 15, 30, 30, UI_BUTTON_BORDER_COLOR, UI_BUTTON_BACKGROUND_COLOR,
                                UI_BUTTON_TEXT_COLOR, "+");
    mainSprite.setFont(&FreeSans9pt7b);

    // Draw brightness
    mainSprite.drawString("Brightness", xLabels, y[4]);
    mainSprite.drawString(formatString("%d %%", sharedState.getBrightness()).c_str(), xValues, y[4]);
    sliderBrightness.draw(&mainSprite, 170, y[4] - 5, 130, 10, UI_SLIDER_BORDER_COLOR, UI_SLIDER_BACKGROUND_COLOR,
                          UI_SLIDER_THUMB_COLOR);

    mainSprite.pushSprite(&display, 0, 0);
    display.endTransaction();
}

/** Draw the info screen, showing static values (satellites, total dist., ...) */
void drawInfoScreen() {
    display.beginTransaction();
    mainSprite.fillSprite(WHITE);
    mainSprite.setTextColor(BLACK);

    // Screen title
    mainSprite.setFont(&FreeSansBold9pt7b);
    mainSprite.drawString("Info", 10, 6);
    mainSprite.setFont(&FreeSans9pt7b);
    mainSprite.drawString("2/2", 290, 6);
    mainSprite.drawFastHLine(0, 27, 320, COLOR_LIGHTGREY);

    uint16_t xLabels = 140;
    uint16_t xValues = xLabels + 20;
    constexpr uint16_t y[]{40, 90, 125, 175, 210};

    // Draw the info labels
    mainSprite.setTextDatum(top_right);
    mainSprite.drawString("GPS satellites:", xLabels, y[0]);
    mainSprite.drawString("Total distance:", xLabels, y[1]);
    mainSprite.drawString("Max speed:", xLabels, y[2]);
    mainSprite.drawString("Temperature:", xLabels, y[3]);
    mainSprite.drawString("Altitude:", xLabels, y[4]);

    // Draw the info values
    mainSprite.setTextDatum(top_left);
    mainSprite.drawNumber(sharedState.getNbSatellites(), xValues, y[0]);
    mainSprite.drawString(formatString("%.1f km", sharedState.getTotalDistance() / 1000).c_str(), xValues, y[1]);
    mainSprite.drawString(formatString("%.1f km/h", sharedState.getMaxSpeed()).c_str(), xValues, y[2]);
    mainSprite.drawString(formatString("%.1f *C", sharedState.getTemperature()).c_str(), xValues, y[3]);
    mainSprite.drawString(formatString("%.1f m", sharedState.getAltitude()).c_str(), xValues, y[4]);

    mainSprite.pushSprite(&display, 0, 0);
    display.endTransaction();
}

void initParameterComponents(uint8_t brightness) {
    // Stage distance
    buttonStageIncrease.setClickHandler([]() { sharedState.addToStageDistance(BUTTON_INCREMENT_DISTANCE_M); });
    buttonStageDecrease.setClickHandler([]() { sharedState.addToStageDistance(-BUTTON_INCREMENT_DISTANCE_M); });
    buttonStageReset.setHoldHandler([]() { sharedState.resetStageDistance(); });

    // Distance mode
    std::vector<RadioButtonOption> options = {
        {WHEEL_SENSOR, "Wheel sensor"},
        {GPS, "GPS"},
    };
    radioButtonDistanceMode.setup(options, WHEEL_SENSOR);
    radioButtonDistanceMode.setChangeHandler(
        [](uint8_t mode) { sharedState.setDistanceMode(static_cast<DistanceMode>(mode)); });

    // Wheel size
    buttonWheelSizeIncrease.setClickHandler([]() { sharedState.addToWheelSize(1); });
    buttonWheelSizeDecrease.setClickHandler([]() { sharedState.addToWheelSize(-1); });

    // Timezone
    buttonTimezoneIncrease.setClickHandler([]() { sharedState.addToTimezone(1); });
    buttonTimezoneDecrease.setClickHandler([]() { sharedState.addToTimezone(-1); });

    // Brightness
    sliderBrightness.setup(0, 100, brightness);
    sliderBrightness.setChangeHandler([](int16_t newValue) {
        sharedState.setBrightness(newValue);
        display.setBrightness(50 + newValue / 2);
    });
}

/**
 * Process for the display. This process listens for touch events and updates the screen accordingly.
 * @param arg Unused.
 */
void displayProcess(void* arg) {
    // Init display and touch
    vTaskDelay(pdMS_TO_TICKS(50));
    auto cfg = M5.config();
    M5.begin(cfg);
    display = M5.Display;

    // Set brightness from saved state
    uint8_t brightness = sharedState.getBrightness();
    display.setBrightness(50 + brightness / 2);

    // Set main screen from saved state
    stateUiMainScreen = static_cast<StateUiMainScreen>(sharedState.getPage());

    // Init main sprite - use 8-bit colors to save memory
    mainSprite.setColorDepth(8);
    mainSprite.createSprite(320, 240);

    initParameterComponents(brightness);

    swipe.setSwipeHandler(swipeHandler);

    while (true) {
        M5.update();
        swipe.update();

        // TODO: update components only if displayed
        buttonStageIncrease.update();
        buttonStageDecrease.update();
        buttonStageReset.update();
        radioButtonDistanceMode.update();
        buttonWheelSizeIncrease.update();
        buttonWheelSizeDecrease.update();
        buttonTimezoneIncrease.update();
        buttonTimezoneDecrease.update();
        sliderBrightness.update(&swipe);

        // Draw according to state
        if (stateUiScreen == MAIN) {
            drawMainScreen();
        } else if (stateUiScreen == PARAMETERS) {
            if (stateUiParametersScreen == PARAMETERS_PAGE_1) {
                drawParametersScreen();
            } else {
                drawInfoScreen();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_LOOP_DELAY_MS));
    }
}
