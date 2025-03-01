#pragma once

#include <M5Unified.h>

#include "constants.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"
/**
 * Process for the magnetic sensor. This process listens for magnetic sensor data and updates the shared state.
 * @param arg Unused.
 */
void magneticProcess(void *arg) {
    // Mode for distance calculation (GPS or wheel sensor). We only change state if the mode is WHEEL_SENSOR.
    DistanceMode mode = sharedState.getDistanceMode();

    // Wheel size in mm for distance calculation
    uint16_t wheel_size = sharedState.getWheelSize();

    // Register as observer for mode and wheel size changes
    sharedState.registerModeObserver(xTaskGetCurrentTaskHandle());
    sharedState.registerWheelSizeObserver(xTaskGetCurrentTaskHandle());

    // Loop forever while
    while (true) {
        // On notification, update the mode and wheel size from the shared state
        if (xTaskNotifyWait(0, 0, NULL, 0) == pdPASS) {
            M5_LOGD("Task `magnetic` received a notification");
            mode = sharedState.getDistanceMode();
            wheel_size = sharedState.getWheelSize();
        }

        vTaskDelay(pdMS_TO_TICKS(MAGNETIC_LOOP_DELAY_MS));
    }
}
