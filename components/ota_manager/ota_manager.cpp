#include "ota_manager.hpp"
#include "esp_log.h"
#include "esp_system.h"

static const char* TAG = "OTA_MGR";

esp_err_t OTAManager::begin() {
    ESP_LOGI(TAG, "Starting OTA Update Session...");
    m_update_partition = esp_ota_get_next_update_partition(nullptr);
    if (!m_update_partition) {
        ESP_LOGE(TAG, "No valid OTA update partition found!");
        return ESP_FAIL;
    }

    esp_err_t err = esp_ota_begin(m_update_partition, OTA_WITH_SEQUENTIAL_WRITES, &m_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (0x%x)", err);
        return err;
    }
    m_in_progress = true;
    return ESP_OK;
}

esp_err_t OTAManager::writeChunk(const void* data, size_t length) {
    if (!m_in_progress) return ESP_ERR_INVALID_STATE;
    return esp_ota_write(m_handle, data, length);
}

esp_err_t OTAManager::endAndReboot() {
    if (!m_in_progress) return ESP_ERR_INVALID_STATE;
    esp_err_t err = esp_ota_end(m_handle);
    if (err != ESP_OK) return err;

    err = esp_ota_set_boot_partition(m_update_partition);
    if (err != ESP_OK) return err;

    m_in_progress = false;
    ESP_LOGI(TAG, "OTA update success! Rebooting in 1 second...");
    esp_restart();
    return ESP_OK;
}

void OTAManager::abort() {
    if (m_in_progress) {
        esp_ota_abort(m_handle);
        m_in_progress = false;
    }
}
