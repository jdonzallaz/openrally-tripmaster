#pragma once

#include <M5Unified.h>

#include "constants.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state.h"

uint32_t lastRevolutionCount{0};

volatile uint32_t revolutionCount{0};
volatile uint64_t lastPulseTime{0};

uint64_t lastSpeedCheckTime{0};  // us
uint32_t lastSpeedCheckRevolutionCount{0};

static void IRAM_ATTR hall_sensor_isr_handler(void *arg) {
    uint64_t now = esp_timer_get_time();
    if (now - lastPulseTime < MAGNETIC_MIN_ISR_DELAY_US) {
        return;
    }
    revolutionCount++;
    lastPulseTime = now;
}

void updateDistance(uint16_t wheel_size) {
    // Save all variables first to avoid concurrent access
    uint32_t tmpRevolutionCount = revolutionCount;

    uint32_t revolutionDifference = tmpRevolutionCount - lastRevolutionCount;
    if (revolutionDifference > 0 && revolutionDifference < MAGNETIC_UPDATE_MAX_REVOLUTIONS) {
        float incrementalDistance = revolutionDifference * wheel_size / 1000.0f;
        sharedState.addToStageDistance(incrementalDistance);
        lastRevolutionCount = tmpRevolutionCount;
    }
}

float calculateSpeed(uint16_t wheel_size) {
    // Save all variables first to avoid concurrent access
    uint32_t tmpRevolutionCount = revolutionCount;
    int64_t tmpLastPulseTime = lastPulseTime;

    int64_t timeDifference = tmpLastPulseTime - lastSpeedCheckTime;
    int32_t revolutionDifference = tmpRevolutionCount - lastSpeedCheckRevolutionCount;
    lastSpeedCheckTime = tmpLastPulseTime;
    lastSpeedCheckRevolutionCount = tmpRevolutionCount;

    if (lastSpeedCheckTime > 0 && timeDifference > 0 && revolutionDifference > 0 &&
        revolutionDifference < MAGNETIC_UPDATE_MAX_REVOLUTIONS) {
        // Calculate speed in km/h
        return (wheel_size * revolutionDifference) / (timeDifference / 3600.0f);
    } else {
        return 0.0f;
    }
}

void attachInterrupt() {
    // Configure GPIO for hall sensor
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << MAGNETIC_SENSOR_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,  // TODO: Check if this is correct
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,  // Trigger on rising edge TODO: Check if this is correct
    };
    gpio_config(&io_conf);

    // Hook isr handler for specific gpio pin
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);  // TODO: check error
    gpio_isr_handler_add(MAGNETIC_SENSOR_PIN, hall_sensor_isr_handler, (void *)MAGNETIC_SENSOR_PIN);
}

/**
 * Process for the magnetic sensor. This process listens for magnetic sensor data and updates the shared state.
 * @param arg Unused.
 */
void magneticProcess(void *arg) {
    // Create interrupt for magnetic sensor
    attachInterrupt();

    // Mode for distance calculation (GPS or wheel sensor). We only change state if the mode is WHEEL_SENSOR.
    DistanceMode mode = sharedState.getDistanceMode();

    // Wheel size in mm for distance calculation
    uint16_t wheel_size = sharedState.getWheelSize();

    // Register as observer for mode and wheel size changes
    sharedState.registerModeObserver(xTaskGetCurrentTaskHandle());
    sharedState.registerWheelSizeObserver(xTaskGetCurrentTaskHandle());

    uint8_t countSpeedIterations{0};

    // Loop forever while
    while (true) {
        // On notification, update the mode and wheel size from the shared state
        if (xTaskNotifyWait(0, 0, NULL, 0) == pdPASS) {
            M5_LOGD("Task `magnetic` received a notification");
            mode = sharedState.getDistanceMode();
            wheel_size = sharedState.getWheelSize();

            if (mode == WHEEL_SENSOR) {
                // Reset the last revolution count and speed check variables
                lastRevolutionCount = revolutionCount = lastSpeedCheckRevolutionCount = 0;
                lastSpeedCheckTime = lastPulseTime = 0;
            }
        }

        if (mode == WHEEL_SENSOR) {
            updateDistance(wheel_size);

            if (countSpeedIterations++ % (MAGNETIC_SPEED_CHECK_DELAY_MS / MAGNETIC_LOOP_DELAY_MS) == 0) {
                sharedState.setSpeed(calculateSpeed(wheel_size));
                countSpeedIterations = 1;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(MAGNETIC_LOOP_DELAY_MS));
    }
}
