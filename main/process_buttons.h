#pragma once

#include <M5Unified.h>
#include <iot_button.h>

#include "constants.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"

/**
 * Increment the waypoint distance by BUTTON_INCREMENT_DISTANCE_M.
 */
static void incrementWaypointDistance(void *arg, void *usr_data) {
    sharedState.addToStageDistance(BUTTON_INCREMENT_DISTANCE_M);
}

/**
 * Decrement the waypoint distance by BUTTON_DECREMENT_DISTANCE_M.
 */
static void decrementWaypointDistance(void *arg, void *usr_data) {
    sharedState.addToStageDistance(BUTTON_DECREMENT_DISTANCE_M);
}

/**
 * Reset the waypoint distance.
 */
static void resetWaypointDistance(void *arg, void *usr_data) { sharedState.resetStageDistance(); }

/**
 * Change main screen mode.
 */
static void changeMainScreen(void *arg, void *usr_data) {
    xTaskNotify((TaskHandle_t)usr_data, NULL, eSetValueWithOverwrite);
}

/**
 * Process for the physical buttons. This process listens for button presses and interacts with the state.
 * @param arg Unused.
 */
void buttonsProcess(void *arg) {
    // Set display task handle from arg
    TaskHandle_t displayTaskHandle = (TaskHandle_t)arg;

    // Create increment gpio button
    button_config_t gpio_increment_btn_cfg = {
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
    button_handle_t gpio_increment_btn = iot_button_create(&gpio_increment_btn_cfg);
    if (NULL == gpio_increment_btn) {
        M5_LOGE("GPIO increment button creation failed");
    }
    // Register button callbacks for the increment waypoint distance button
    iot_button_register_cb(gpio_increment_btn, BUTTON_PRESS_DOWN, incrementWaypointDistance, NULL);
    iot_button_register_cb(gpio_increment_btn, BUTTON_LONG_PRESS_HOLD, incrementWaypointDistance, NULL);

    // Create decrement gpio button
    button_config_t gpio_decrement_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = BUTTON_DECREMENT_DISTANCE_LONG_PRESS_TIME_MS,
        .short_press_time = BUTTON_DECREMENT_DISTANCE_SHORT_PRESS_TIME_MS,
        .gpio_button_config =
            {
                .gpio_num = BUTTON_DECREMENT_DISTANCE_GPIO,
                .active_level = 0,
                .disable_pull = false,
            },
    };
    button_handle_t gpio_decrement_btn = iot_button_create(&gpio_decrement_btn_cfg);
    if (NULL == gpio_decrement_btn) {
        M5_LOGE("GPIO decrement button creation failed");
    }
    // Register button callbacks for the decrement waypoint distance button
    iot_button_register_cb(gpio_decrement_btn, BUTTON_PRESS_DOWN, decrementWaypointDistance, NULL);
    iot_button_register_cb(gpio_decrement_btn, BUTTON_LONG_PRESS_HOLD, decrementWaypointDistance, NULL);

    // Create menu gpio button
    button_config_t gpio_menu_btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = BUTTON_MENU_LONG_PRESS_TIME_MS,
        .short_press_time = BUTTON_MENU_SHORT_PRESS_TIME_MS,
        .gpio_button_config =
            {
                .gpio_num = BUTTON_MENU_GPIO,
                .active_level = 0,
                .disable_pull = false,
            },
    };
    button_handle_t gpio_menu_btn = iot_button_create(&gpio_menu_btn_cfg);
    if (NULL == gpio_menu_btn) {
        M5_LOGE("GPIO menu button creation failed");
    }
    // Register button callbacks for the menu button
    iot_button_register_cb(gpio_menu_btn, BUTTON_SINGLE_CLICK, changeMainScreen, displayTaskHandle);
    iot_button_register_cb(gpio_menu_btn, BUTTON_LONG_PRESS_START, resetWaypointDistance, NULL);

    // Loop forever while processing button events
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(BUTTONS_LOOP_DELAY_MS));
    }
}
