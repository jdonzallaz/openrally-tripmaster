#pragma once

#include "constants.h"
#include "freertos/FreeRTOS.h"

/**
 * Process for the magnetic sensor. This process listens for magnetic sensor data and updates the shared state.
 * @param arg Unused.
 */
void magneticProcess(void *arg) {
    // Loop forever while
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(MAGNETIC_LOOP_DELAY_MS));
    }
}
