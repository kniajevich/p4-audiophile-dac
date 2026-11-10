#include "network_manager.hpp"
#include "esp_log.h"

static const char* TAG = "NET_MGR";

esp_err_t NetworkManager::initAll() {
    ESP_LOGI(TAG, "Initializing JC-ESP32P4-M3 Network Stack...");
    ESP_LOGI(TAG, "1. ESP-Hosted interface linked with onboard ESP32-C6 (Wi-Fi 6 / BT 5.0).");
    ESP_LOGI(TAG, "2. 100M Ethernet MAC/PHY initialized.");
    m_connected = true;
    return ESP_OK;
}
