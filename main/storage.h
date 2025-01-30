#pragma once

#include <M5Unified.h>
#include <esp_err.h>

#include "nvs_flash.h"

/** Initialize the NVS storage. */
void initStorage() {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased -> retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    M5_LOGI("NVS storage initalized");
}

/** Erase the NVS storage. */
void cleanStorage() { ESP_ERROR_CHECK(nvs_flash_erase()); }
