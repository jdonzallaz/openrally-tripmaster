#pragma once

#include <M5Unified.h>

#include "constants.h"
#include "freertos/FreeRTOS.h"
#include "state.h"

/**
 * Process for the temperature module. This process checks the internal temperature of the device and updates the shared
 * state.
 * @param arg Unused.
 */
void TemperatureProcess(void *arg) {
    // Delay before starting the temperature process to allow proper initialization of the hardware
    vTaskDelay(pdMS_TO_TICKS(TEMPERATURE_PROCESS_INITIALIZATION_DELAY));

    // Loop forever while reading the temperature
    while (true) {
        // Get temperature from the power management IC of the device.
        float temperature = M5.Power.Axp192.getInternalTemperature();
        sharedState.setTemperature(temperature);
        M5_LOGD("Temperature: %.2f °C", temperature);

        vTaskDelay(pdMS_TO_TICKS(TEMPERATURE_LOOP_DELAY_MS));
    }
}
