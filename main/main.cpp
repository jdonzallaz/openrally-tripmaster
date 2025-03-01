#include "process_buttons.h"
#include <M5Unified.h>

#include "constants.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "process_buttons.h"
#include "process_display.h"
#include "process_gps.h"
#include "process_temperature.h"
#include "state.h"
#include "storage.h"

/**
 * Application entry point. Initialize the M5Stack and create the processes.
 */
extern "C" void app_main() {
    // Delay before starting the application to allow proper initialization of the hardware
    vTaskDelay(pdMS_TO_TICKS(50));

    initStorage();
    sharedState.loadData();

    // Start the display process
    BaseType_t result = xTaskCreatePinnedToCore(displayProcess, "DisplayProcess", DISPLAY_PROCESS_STACK_DEPTH, NULL,
                                                DISPLAY_PROCESS_PRIORITY, NULL, DISPLAY_PROCESS_CORE);
    if (result != pdPASS) {
        M5_LOGE("Failed to create DisplayProcess %s", esp_err_to_name(result));
    }

    // Start the GPS process
    result = xTaskCreatePinnedToCore(gpsProcess, "GPSProcess", GPS_PROCESS_STACK_DEPTH, NULL, GPS_PROCESS_PRIORITY,
                                     NULL, GPS_PROCESS_CORE);
    if (result != pdPASS) {
        M5_LOGE("Failed to create GPSProcess %s", esp_err_to_name(result));
    }

    // Start the buttons process
    result = xTaskCreatePinnedToCore(buttonsProcess, "ButtonsProcess", BUTTONS_PROCESS_STACK_DEPTH, NULL,
                                     BUTTONS_PROCESS_PRIORITY, NULL, BUTTONS_PROCESS_CORE);
    if (result != pdPASS) {
        M5_LOGE("Failed to create ButtonsProcess %s", esp_err_to_name(result));
    }

    // Start the temperature process
    result = xTaskCreatePinnedToCore(TemperatureProcess, "TemperatureProcess", TEMPERATURE_PROCESS_STACK_DEPTH, NULL,
                                     TEMPERATURE_PROCESS_PRIORITY, NULL, TEMPERATURE_PROCESS_CORE);
    if (result != pdPASS) {
        M5_LOGE("Failed to create TemperatureProcess %s", esp_err_to_name(result));
    }
}
