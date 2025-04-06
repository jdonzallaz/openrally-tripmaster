#pragma once

#include <M5Unified.h>
#include <stddef.h>
#include <stdint.h>

#include "constants.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"

/** Saveable value template, with key for storage, value, and whether it's dirty and must be saved to storage.  */
template <typename T>
struct SaveableValue {
    T value;
    const char* key;
    bool isDirty = false;
};

struct Time {
    uint8_t hour{0};
    uint8_t minute{0};
    uint8_t second{0};
};

enum DistanceMode : uint8_t {
    WHEEL_SENSOR,  // Distance and speed calculated from wheel sensor
    GPS,           // Distance and speed calculated from GPS
};

class SharedState {
    // Distance traveled in meters in the stage. Saved. Configurable (+, -, reset).
    SaveableValue<float> stageDistance{0.0f, "stageDistance"};
    // Total distance traveled in meters. Saved.
    SaveableValue<float> totalDistance{0.0f, "totalDistance"};

    // Direction (cap) in degrees
    uint16_t cap{0};
    // Speed in km/h
    float speed{0.0f};
    // Max speed in km/h. Saved.
    SaveableValue<float> maxSpeed{0.0f, "maxSpeed"};
    // Altitude in meters
    float altitude{0.0f};
    // Number of GPS satellites connected
    uint8_t nbSatellites{0};

    // Current time
    Time time;
    // Timezone offset in hours. Saved. Configurable (+, -).
    SaveableValue<int8_t> timezone{0, "timezone"};

    // Temperature in degrees celsius
    float temperature{0.0f};

    // Mode for distance calculation (GPS or wheel sensor). Saved. Configurable.
    SaveableValue<DistanceMode> distanceMode{WHEEL_SENSOR, "distanceMode"};
    // Wheel size in mm. Saved. Configurable (+, -).
    SaveableValue<uint16_t> wheelSize{STATE_DEFAULT_WHEEL_SIZE, "wheelSize"};

    // Brightness level of the screen (0-100). Saved. Configurable (+, -).
    SaveableValue<uint8_t> brightness{STATE_DEFAULT_BRIGHTNESS, "brightness"};
    // Current page on main screen. Saved.
    SaveableValue<uint8_t> page{0, "page"};

    // Whether data has been updated and should be saved to storage
    bool isDirty{false};

    // Mutex for state access
    SemaphoreHandle_t mutex;

    // Observers for mode and wheel size changes
    TaskHandle_t modeObservers[STATE_MAX_OBSERVERS]{nullptr};
    TaskHandle_t wheelSizeObservers[STATE_MAX_OBSERVERS]{nullptr};

    // Saving variables
    // Flag: is currently riding - moving at speed >~ 25km/h
    bool isRiding{false};
    float lastDirtyDistance{0.0f};
    uint64_t lastSaveTime{0ULL};
    esp_timer_handle_t debouncedSaveTimer{nullptr};

    /**
     * Write the state to NVS (only if the value is dirty).
     * @param nvsHandle NVS handle to write to.
     */
    void writeToNvs(const nvs_handle_t& nvsHandle) {
        if (stageDistance.isDirty) {
            nvs_set_blob(nvsHandle, stageDistance.key, &stageDistance.value, sizeof(float));
        }
        if (totalDistance.isDirty) {
            nvs_set_blob(nvsHandle, totalDistance.key, &totalDistance.value, sizeof(float));
        }
        if (maxSpeed.isDirty) {
            nvs_set_blob(nvsHandle, maxSpeed.key, &maxSpeed.value, sizeof(float));
        }
        if (timezone.isDirty) {
            nvs_set_i8(nvsHandle, timezone.key, timezone.value);
        }
        if (distanceMode.isDirty) {
            nvs_set_u8(nvsHandle, distanceMode.key, distanceMode.value);
        }
        if (wheelSize.isDirty) {
            nvs_set_u16(nvsHandle, wheelSize.key, wheelSize.value);
        }
        if (brightness.isDirty) {
            nvs_set_u8(nvsHandle, brightness.key, brightness.value);
        }
        if (page.isDirty) {
            nvs_set_u8(nvsHandle, page.key, page.value);
        }
    }

    /**
     * Read the state from NVS.
     * @param nvsHandle NVS handle to read from.
     */
    void readFromNvs(const nvs_handle_t& nvsHandle) {
        size_t floatSize = sizeof(float);

        nvs_get_blob(nvsHandle, stageDistance.key, &stageDistance.value, &floatSize);
        nvs_get_blob(nvsHandle, totalDistance.key, &totalDistance.value, &floatSize);
        nvs_get_blob(nvsHandle, maxSpeed.key, &maxSpeed.value, &floatSize);
        nvs_get_i8(nvsHandle, timezone.key, &timezone.value);
        nvs_get_u8(nvsHandle, distanceMode.key, (uint8_t*)&distanceMode.value);  // TODO: Switch mode
        nvs_get_u16(nvsHandle, wheelSize.key, &wheelSize.value);
        nvs_get_u8(nvsHandle, brightness.key, &brightness.value);
        nvs_get_u8(nvsHandle, page.key, &page.value);
    }

    /**
     * Create a debounced timer if it doesn't exist. This timer is used to save the state after a delay.
     */
    void createDebouncedTimerIfNotExists() {
        if (debouncedSaveTimer == nullptr) {
            const esp_timer_create_args_t debouncedSaveTimerArgs = {
                .callback = [](void* arg) { static_cast<SharedState*>(arg)->saveData(STATE_DEBOUNCE_DELAY_US); },
                .arg = this,
                .name = "debouncedSave",
            };
            ESP_ERROR_CHECK(esp_timer_create(&debouncedSaveTimerArgs, &debouncedSaveTimer));
        }
    }

    /**
     * Set the saveable state as modified. This will trigger a save after a delay.
     */
    void setSaveableStateModified() {
        isDirty = true;

        createDebouncedTimerIfNotExists();

        if (esp_timer_is_active(debouncedSaveTimer)) {
            esp_timer_stop(debouncedSaveTimer);
        }

        M5_LOGD("State: Debouncing save");
        esp_timer_start_once(debouncedSaveTimer, STATE_DEBOUNCE_DELAY_US);
    }

   public:
    SharedState() {
        mutex = xSemaphoreCreateMutex();
        if (mutex == NULL) {
            M5_LOGE("Failed to create state mutex");
            abort();
        }
    }

    ~SharedState() {
        if (mutex) {
            vSemaphoreDelete(mutex);
        }
    }

    /**
     * Save the state to NVS if it has been updated and the interval has passed.
     * @param ignoreDelay Minimum delay between saves in microseconds.
     */
    void saveData(uint32_t ignoreDelay = STATE_MIN_SAVE_DELAY_US) {
        uint64_t now = esp_timer_get_time();

        // Ignore saving if interval too short, or if there is nothing to save
        if (now - lastSaveTime < ignoreDelay || !isDirty) {
            M5_LOGD("State: Too early or not dirty -> no saving");
            return;
        }

        M5_LOGD("State: Saving");

        // Open NVS
        nvs_handle_t nvsHandle;
        esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
        if (err != ESP_OK) {
            M5_LOGE("Error opening NVS handle: %s", esp_err_to_name(err));
            return;
        }

        // Save
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT) && isDirty) {
            writeToNvs(nvsHandle);
            xSemaphoreGive(mutex);
        }

        // Commit to memory and close
        nvs_commit(nvsHandle);
        nvs_close(nvsHandle);

        isDirty = false;
        lastSaveTime = now;
    }

    /**
     * Load the state from NVS.
     */
    void loadData() {
        // Open NVS
        nvs_handle_t nvsHandle;
        esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvsHandle);

        if (err != ESP_OK) {
            M5_LOGE("Error opening NVS handle: %s", esp_err_to_name(err));
            return;
        }

        // Load
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            readFromNvs(nvsHandle);

            xSemaphoreGive(mutex);
        }

        // Close handle
        nvs_close(nvsHandle);

        M5_LOGI("State data loaded from NVS");
    }

    void addToStageDistance(float distance) {
        if (abs(distance) > STATE_DISTANCE_EPSILON && xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            float oldStageDistance = stageDistance.value;
            stageDistance.value += distance;
            stageDistance.value = stageDistance.value < 0.0f ? 0.0f : stageDistance.value;
            if (distance > STATE_DISTANCE_EPSILON) {
                totalDistance.value += distance;
                totalDistance.isDirty = true;
            }
            M5_LOGD("Set distance: %f to %f", oldStageDistance, stageDistance.value);
            if (abs(stageDistance.value - oldStageDistance) >= STATE_DISTANCE_EPSILON) {
                stageDistance.isDirty = isDirty = true;
            }
            xSemaphoreGive(mutex);
        }
    }

    void resetStageDistance() {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            if (stageDistance.value > 0.01f) {
                stageDistance.value = 0.0f;
                stageDistance.isDirty = true;
                setSaveableStateModified();
            }
            xSemaphoreGive(mutex);
        }
    }

    float getStageDistance() {
        float localCopy{0.0f};  // TODO: what to return if not available?
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = stageDistance.value;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    float getTotalDistance() {
        float localCopy{0.0f};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = totalDistance.value;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void setCap(uint16_t cap) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            this->cap = cap;
            xSemaphoreGive(mutex);
        }
    }

    uint16_t getCap() {
        uint16_t localCopy{0};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = cap;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void setSpeed(float speed) {
        if (speed < STATE_MAX_VALID_SPEED && xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            // Set new speed
            this->speed = speed;

            // Set max speed
            if (speed > maxSpeed.value) {
                maxSpeed.value = speed;
                maxSpeed.isDirty = isDirty = true;
            }

            // Save when stopping after going over "riding" speed
            if (isRiding && speed < STATE_SPEED_EPSILON) {
                M5_LOGD("State: Stopped after riding: %f", speed);
                isRiding = false;
                saveData();
            }

            // Set riding flag if going over "riding" speed
            if (!isRiding && speed > STATE_RIDING_SPEED) {
                M5_LOGD("State: Riding speed reached: %f", speed);
                isRiding = true;
            }

            xSemaphoreGive(mutex);
        }
    }

    float getSpeed() {
        float localCopy{0.0f};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = speed;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    float getMaxSpeed() {
        float localCopy{0.0f};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = maxSpeed.value;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void setAltitude(float altitude) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            this->altitude = altitude;
            xSemaphoreGive(mutex);
        }
    }

    float getAltitude() {
        float localCopy{0.0f};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = altitude;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void setNbSatellites(uint8_t nbSatellites) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            this->nbSatellites = nbSatellites;
            xSemaphoreGive(mutex);
        }
    }

    uint8_t getNbSatellites() {
        uint8_t localCopy{0};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = nbSatellites;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void setTime(uint8_t hour, uint8_t minute, uint8_t second) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            time.hour = (hour + timezone.value) % 24;
            time.minute = minute;
            time.second = second;
            xSemaphoreGive(mutex);
        }
    }

    Time getTime() {
        Time localCopy;
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = time;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void setTimezone(int8_t timezone) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            this->timezone.value = timezone;
            this->timezone.isDirty = true;
            setSaveableStateModified();
            xSemaphoreGive(mutex);
        }
    }

    void addToTimezone(int8_t hour) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            timezone.value += hour;
            timezone.value = timezone.value < -12 ? -12 : timezone.value > 14 ? 14 : timezone.value;
            timezone.isDirty = true;
            setSaveableStateModified();
            xSemaphoreGive(mutex);
        }
    }

    int8_t getTimezone() {
        int8_t localCopy{0};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = timezone.value;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void setTemperature(float temperature) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            this->temperature = temperature;
            xSemaphoreGive(mutex);
        }
    }

    float getTemperature() {
        float localCopy{0.0f};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = temperature;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void setDistanceMode(DistanceMode distanceMode) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            this->distanceMode.value = distanceMode;
            this->distanceMode.isDirty = true;
            setSaveableStateModified();

            // Notify all registered tasks for mode change
            for (TaskHandle_t task : modeObservers) {
                if (task != nullptr) {
                    xTaskNotify(task, distanceMode, eSetValueWithOverwrite);
                }
            }

            xSemaphoreGive(mutex);
        }
    }

    DistanceMode getDistanceMode() {
        DistanceMode localCopy{WHEEL_SENSOR};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = distanceMode.value;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void addToWheelSize(int16_t size) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            if (wheelSize.value + size < 0) {
                wheelSize.value = 0;
            } else {
                wheelSize.value += size;
            }
            this->wheelSize.isDirty = true;
            setSaveableStateModified();

            // Notify all registered tasks for wheel size change
            for (TaskHandle_t task : wheelSizeObservers) {
                if (task != nullptr) {
                    xTaskNotify(task, wheelSize.value, eSetValueWithOverwrite);
                }
            }

            xSemaphoreGive(mutex);
        }
    }

    uint16_t getWheelSize() {
        uint16_t localCopy{0};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = wheelSize.value;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void setBrightness(uint8_t brightness) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            this->brightness.value = brightness;
            this->brightness.value = this->brightness.value > 100 ? 100 : this->brightness.value;
            this->brightness.isDirty = true;
            setSaveableStateModified();
            xSemaphoreGive(mutex);
        }
    }

    uint8_t getBrightness() {
        uint8_t localCopy{100};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = brightness.value;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    void setPage(uint8_t page) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            this->page.value = page;
            this->page.isDirty = true;
            xSemaphoreGive(mutex);
        }
    }

    uint8_t getPage() {
        uint8_t localCopy{0};
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            localCopy = page.value;
            xSemaphoreGive(mutex);
        }
        return localCopy;
    }

    // Register observer for mode changes
    bool registerModeObserver(TaskHandle_t task) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            for (int i = 0; i < STATE_MAX_OBSERVERS; i++) {
                if (modeObservers[i] == nullptr) {
                    modeObservers[i] = task;
                    xSemaphoreGive(mutex);
                    return true;
                }
            }
            xSemaphoreGive(mutex);
        }
        return false;  // Observer list is full
    }

    // Register observer for wheel size changes
    bool registerWheelSizeObserver(TaskHandle_t task) {
        if (xSemaphoreTake(mutex, STATE_SEMAPHORE_TIMEOUT)) {
            for (int i = 0; i < STATE_MAX_OBSERVERS; i++) {
                if (wheelSizeObservers[i] == nullptr) {
                    wheelSizeObservers[i] = task;
                    xSemaphoreGive(mutex);
                    return true;
                }
            }
            xSemaphoreGive(mutex);
        }
        return false;  // Observer list is full
    }
} sharedState;

/**
 * Process for the storage. This process saves the state to NVS at regular intervals.
 * @param arg Unused.
 */
void storageProcess() {
    // Create and start the periodic timer -> save every 3min
    const esp_timer_create_args_t periodic_save_timer_args = {
        .callback = [](void* arg) { sharedState.saveData(); },
        .name = "periodicSave",
    };
    esp_timer_handle_t periodic_save_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_save_timer_args, &periodic_save_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_save_timer, STATE_SAVE_LOOP_DELAY_US));
}
