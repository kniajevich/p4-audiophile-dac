#include "network_manager.hpp"
#include <cstring>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/ip4_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char* TAG = "NET_MGR";

#define DIRECT_AP_CHANNEL   6
#define DIRECT_AP_MAX_CONN  4
#define STA_CONNECT_TIMEOUT_MS 5000

static EventGroupHandle_t s_wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_FAIL_BIT      = BIT1;

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
            ESP_LOGI(TAG, "Loaded stored Client Wi-Fi: %s", m_sta_ssid.c_str());
        }

        char buf_ap_ssid[64] = {0};
        char buf_ap_pass[64] = {0};
        size_t len_ap_ssid = sizeof(buf_ap_ssid);
        size_t len_ap_pass = sizeof(buf_ap_pass);
        if (nvs_get_str(nvs, "ap_ssid", buf_ap_ssid, &len_ap_ssid) == ESP_OK &&
            nvs_get_str(nvs, "ap_pass", buf_ap_pass, &len_ap_pass) == ESP_OK) {
            m_ap_ssid = buf_ap_ssid;
            m_ap_pass = buf_ap_pass;
            ESP_LOGI(TAG, "Loaded custom SoftAP config: %s", m_ap_ssid.c_str());
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
        ESP_LOGI(TAG, "Custom SoftAP persistently saved to NVS");
    }
}

esp_err_t NetworkManager::setCredentials(const std::string& ssid, const std::string& password) {
    saveCredentials(ssid, password);
    return startSTA();
}

esp_err_t NetworkManager::setAPCredentials(const std::string& ssid, const std::string& password) {
    if (ssid.empty() || password.length() < 8) return ESP_ERR_INVALID_ARG;
    saveAPCredentials(ssid, password);
    if (m_is_ap) {
        return startDirectAP();
    }
    return ESP_OK;
}

void NetworkManager::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    NetworkManager* self = static_cast<NetworkManager*>(arg);

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                self->m_connected = false;
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "Client joined SoftAP");
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            auto* event = (ip_event_got_ip_t*)event_data;
            char ip[16];
            esp_ip4addr_ntoa(&event->ip_info.ip, ip, sizeof(ip));
            self->m_ip_str = ip;
            self->m_connected = true;
            self->m_is_ap = false;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            ESP_LOGI(TAG, "Station connected! Local IP: %s", ip);
        }
    }
}

esp_err_t NetworkManager::init(NetworkMode mode) {
    m_mode = mode;
    s_wifi_event_group = xEventGroupCreate();

    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    loadCredentials();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    m_netif_sta = esp_netif_create_default_wifi_sta();
    m_netif_ap  = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, this, nullptr));

    if (m_mode == NetworkMode::DIRECT_AP || m_sta_ssid.empty()) {
        return startDirectAP();
    }

    ESP_LOGI(TAG, "Attempting STA connection to %s (Timeout: %d ms)...", m_sta_ssid.c_str(), STA_CONNECT_TIMEOUT_MS);
    esp_err_t ret = startSTA();
    if (ret != ESP_OK) return startDirectAP();

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(STA_CONNECT_TIMEOUT_MS));
    if (bits & WIFI_CONNECTED_BIT) return ESP_OK;

    ESP_LOGW(TAG, "Home router unreachable. Falling back to Direct SoftAP...");
    return startDirectAP();
}

esp_err_t NetworkManager::startDirectAP() {
    esp_wifi_stop();

    wifi_config_t ap_config = {};
    strncpy((char*)ap_config.ap.ssid, m_ap_ssid.c_str(), sizeof(ap_config.ap.ssid) - 1);
    strncpy((char*)ap_config.ap.password, m_ap_pass.c_str(), sizeof(ap_config.ap.password) - 1);
    ap_config.ap.ssid_len = m_ap_ssid.length();
    ap_config.ap.channel = DIRECT_AP_CHANNEL;
    ap_config.ap.authmode = WIFI_AUTH_WPA3_PSK;
    ap_config.ap.max_connection = DIRECT_AP_MAX_CONN;
    ap_config.ap.beacon_interval = 100;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40));

    m_is_ap = true;
    m_connected = true;
    m_ip_str = "192.168.4.1";
    m_current_ssid = m_ap_ssid;
    return ESP_OK;
}

esp_err_t NetworkManager::startSTA() {
    if (m_sta_ssid.empty()) return ESP_FAIL;

    esp_wifi_stop();
    wifi_config_t sta_config = {};
    strncpy((char*)sta_config.sta.ssid, m_sta_ssid.c_str(), sizeof(sta_config.sta.ssid) - 1);
    strncpy((char*)sta_config.sta.password, m_sta_pass.c_str(), sizeof(sta_config.sta.password) - 1);
    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT40));

    m_current_ssid = m_sta_ssid;
    return ESP_OK;
}
