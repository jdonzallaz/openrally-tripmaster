#pragma once
#include <M5Unified.h>
#include <stdint.h>

#include "constants.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"
#include "tinygps++/TinyGPS++.cpp"

TinyGPSPlus gps;

struct Position {
    float latitude;
    float longitude;
};

Position oldPosition{0.0f, 0.0f};
uint64_t timeOldPosition{0};  // us

/**
 * Process for the GPS module. This process listens for GPS data on the UART port and updates the shared state.
 * @param arg Unused.
 */
void gpsProcess(void *arg) {
    // Buffer for data read on UART
    uint8_t data[GPS_UART_BUFFER_SIZE];

    // Configure UART for GPS communication
    uart_config_t uart_config = {.baud_rate = GPS_UART_BAUD_RATE,
                                 .data_bits = UART_DATA_8_BITS,
                                 .parity = UART_PARITY_DISABLE,
                                 .stop_bits = UART_STOP_BITS_1,
                                 .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                                 .source_clk = UART_SCLK_DEFAULT};
    uart_driver_install(GPS_UART_PORT_NUM, GPS_UART_BUFFER_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(GPS_UART_PORT_NUM, &uart_config);
    uart_set_pin(GPS_UART_PORT_NUM, GPS_UART_TX_PIN, GPS_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Mode for distance calculation (GPS or wheel sensor). We only change state if the mode is GPS.
    DistanceMode mode = sharedState.getDistanceMode();

    // Register as observer for mode changes
    sharedState.registerModeObserver(xTaskGetCurrentTaskHandle());

    // Loop forever while processing GPS data
    float latitude, longitude;
    while (true) {
        // On notification, update the mode from the shared state
        uint32_t receivedMode{0};
        if (xTaskNotifyWait(0, 0, &receivedMode, 0) == pdPASS) {
            M5_LOGD("Task `GPS` received a notification");
            mode = static_cast<DistanceMode>(receivedMode);
        }

        // Read data from UART
        int len = uart_read_bytes(GPS_UART_PORT_NUM, data, (GPS_UART_BUFFER_SIZE - 1), 20 / portTICK_PERIOD_MS);

        // Process received data
        if (len > 0) {
            data[len] = 0;  // Null-terminate the received data
            M5_LOGD("GPS: Received data: %s", data);

            // Feed data into the TinyGPS++ object
            for (int i = 0; i < len; i++) {
                gps.encode(data[i]);
            }

            // Time
            if (gps.time.isValid() && gps.time.isUpdated()) {
                sharedState.setTime(gps.time.hour(), gps.time.minute(), gps.time.second());
            }

            // Connected satellites
            if (gps.satellites.isValid() && gps.satellites.isUpdated()) {
                sharedState.setNbSatellites(gps.satellites.value());
            }

            // Altitude
            if (gps.altitude.isValid() && gps.altitude.isUpdated()) {
                sharedState.setAltitude(gps.altitude.meters());
            }

            // Latitude, longitude coordinates
            if (gps.location.isValid() && gps.location.isUpdated()) {
                latitude = gps.location.lat();
                longitude = gps.location.lng();

                // Get current time to compare with last position
                uint64_t now = esp_timer_get_time();
                uint64_t durationSinceLastCheck = now - timeOldPosition;
                if (mode == GPS && timeOldPosition > 0 && durationSinceLastCheck > GPS_UPDATE_MIN_TIME &&
                    durationSinceLastCheck < GPS_UPDATE_MAX_TIME) {
                    // Compute distance in meters
                    float distance =
                        TinyGPSPlus::distanceBetween(oldPosition.latitude, oldPosition.longitude, latitude, longitude);

                    // Update shared state if within reasonable bounds
                    if (distance > GPS_UPDATE_MIN_DISTANCE && distance < GPS_UPDATE_MAX_DISTANCE) {
                        sharedState.addToStageDistance(distance);
                    }
                }

                timeOldPosition = now;
                oldPosition = {latitude, longitude};
            }

            // Speed
            if (mode == GPS && gps.speed.isValid() && gps.speed.isUpdated()) {
                sharedState.setSpeed(gps.speed.kmph());
            }

            // Cap
            if (gps.course.isValid() && gps.course.isUpdated()) {
                sharedState.setCap(gps.course.deg());
            }
        }

        vTaskDelay(pdMS_TO_TICKS(GPS_LOOP_DELAY_MS));
    }
}
