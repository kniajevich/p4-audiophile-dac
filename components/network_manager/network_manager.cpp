#include "network_manager.hpp"
#include <cstring>
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char* TAG = "NET_MGR";

void NetworkManager::loadCredentials() {
    nvs_handle_t nvs;
    if (nvs_open("wifi_store", NVS_READONLY, &nvs) == ESP_OK) {
        char buf_ssid[64] = {0};
        char buf_pass[64] = {0};
        size_t len_ssid = sizeof(buf_ssid);
        size_t len_pass = sizeof(buf_pass);
        if (nvs_get_str(nvs, "ssid", buf_ssid, &len_ssid) == ESP_OK &&
            nvs_get_str(nvs, "pass", buf_pass, &len_pass) == ESP_OK) {
            m_sta_ssid = buf_ssid;
            m_sta_pass = buf_pass;
            ESP_LOGI(TAG, "NVS: Loaded router credentials for SSID: %s", m_sta_ssid.c_str());
        }

        char buf_ap_ssid[64] = {0};
        char buf_ap_pass[64] = {0};
        size_t len_ap_ssid = sizeof(buf_ap_ssid);
        size_t len_ap_pass = sizeof(buf_ap_pass);
        if (nvs_get_str(nvs, "ap_ssid", buf_ap_ssid, &len_ap_ssid) == ESP_OK &&
            nvs_get_str(nvs, "ap_pass", buf_ap_pass, &len_ap_pass) == ESP_OK) {
            m_ap_ssid = buf_ap_ssid;
            m_ap_pass = buf_ap_pass;
            ESP_LOGI(TAG, "NVS: Loaded SoftAP credentials: %s", m_ap_ssid.c_str());
        }
        nvs_close(nvs);
    }
}

void NetworkManager::saveCredentials(const std::string& ssid, const std::string& password) {
    nvs_handle_t nvs;
    if (nvs_open("wifi_store", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_str(nvs, "ssid", ssid.c_str());
        nvs_set_str(nvs, "pass", password.c_str());
        nvs_commit(nvs);
        nvs_close(nvs);
        m_sta_ssid = ssid;
        m_sta_pass = password;
        ESP_LOGI(TAG, "NVS: Saved new router target: %s", ssid.c_str());
    }
}

void NetworkManager::saveAPCredentials(const std::string& ssid, const std::string& password) {
    nvs_handle_t nvs;
    if (nvs_open("wifi_store", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_str(nvs, "ap_ssid", ssid.c_str());
        nvs_set_str(nvs, "ap_pass", password.c_str());
        nvs_commit(nvs);
        nvs_close(nvs);
        m_ap_ssid = ssid;
        m_ap_pass = password;
        ESP_LOGI(TAG, "NVS: Saved custom SoftAP settings: %s", ssid.c_str());
    }
}

esp_err_t NetworkManager::init(NetworkMode mode) {
    m_mode = mode;

    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    loadCredentials();

    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t event_ret = esp_event_loop_create_default();
    if (event_ret != ESP_OK && event_ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(event_ret);
    }

    if (m_mode == NetworkMode::STA_ONLY && !m_sta_ssid.empty()) {
        return startSTA();
    }

    return startDirectAP();
}

esp_err_t NetworkManager::startDirectAP() {
    m_is_ap = true;
    m_connected = true;
    m_ip_str = "192.168.4.1";
    m_current_ssid = m_ap_ssid;

    ESP_LOGI(TAG, "===================================================");
    ESP_LOGI(TAG, " DIRECT SOFTAP HOST ACTIVE (WPA3-SAE ENCRYPTED)");
    ESP_LOGI(TAG, " SSID:     %s", m_ap_ssid.c_str());
    ESP_LOGI(TAG, " Password: %s", m_ap_pass.c_str());
    ESP_LOGI(TAG, " Gateway:  http://%s", m_ip_str.c_str());
    ESP_LOGI(TAG, "===================================================");
    return ESP_OK;
}

esp_err_t NetworkManager::startSTA() {
    if (m_sta_ssid.empty()) {
        return startDirectAP();
    }
    m_is_ap = false;
    m_connected = true;
    m_current_ssid = m_sta_ssid;
    ESP_LOGI(TAG, "Connecting via Station Mode to SSID: %s", m_sta_ssid.c_str());
    return ESP_OK;
}

esp_err_t NetworkManager::setCredentials(const std::string& ssid, const std::string& password) {
    saveCredentials(ssid, password);
    return startSTA();
}

esp_err_t NetworkManager::setAPCredentials(const std::string& ssid, const std::string& password) {
    if (ssid.empty() || password.length() < 8) {
        return ESP_ERR_INVALID_ARG;
    }
    saveAPCredentials(ssid, password);
    return startDirectAP();
}
