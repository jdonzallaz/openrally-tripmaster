#pragma once

#include <M5Unified.h>
#include <iot_button.h>

#include "constants.h"
#include "freertos/FreeRTOS.h"
#include "state.h"

/**
 * Increment the waypoint distance by BUTTON_INCREMENT_DISTANCE_M.
 */
static void incrementWaypointDistance(void *arg, void *usr_data) {
    sharedState.addToStageDistance(BUTTON_INCREMENT_DISTANCE_M);
}

/**
 * Process for the physical buttons. This process listens for button presses and interacts with the state.
 * @param arg Unused.
 */
void buttonsProcess(void *arg) {
    // Create gpio button
    button_config_t gpio_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = BUTTON_INCREMENT_DISTANCE_LONG_PRESS_TIME_MS,
        .short_press_time = BUTTON_INCREMENT_DISTANCE_SHORT_PRESS_TIME_MS,
        .gpio_button_config =
            {
                .gpio_num = BUTTON_INCREMENT_DISTANCE_GPIO,
                .active_level = 0,
                .disable_pull = false,
            },
    };
    button_handle_t gpio_btn = iot_button_create(&gpio_btn_cfg);
    if (NULL == gpio_btn) {
        M5_LOGE("GPIO Button creation failed");
    }

    // Register button callbacks for the increment waypoint distance button
    iot_button_register_cb(gpio_btn, BUTTON_PRESS_DOWN, incrementWaypointDistance, NULL);
    iot_button_register_cb(gpio_btn, BUTTON_LONG_PRESS_HOLD, incrementWaypointDistance, NULL);

    // Loop forever while processing button events
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(BUTTONS_LOOP_DELAY_MS));
    }
}
