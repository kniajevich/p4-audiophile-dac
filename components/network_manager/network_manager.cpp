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

#define DIRECT_AP_SSID      "JC-P4-DirectDAC"
#define DIRECT_AP_PASS      "audiophile123"    // WPA3-SAE пароль
#define DIRECT_AP_CHANNEL   6
#define DIRECT_AP_MAX_CONN  4

#define STA_CONNECT_TIMEOUT_MS 5000

static EventGroupHandle_t s_wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_FAIL_BIT      = BIT1;

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
                ESP_LOGW(TAG, "Station disconnected from AP");
                break;
            case WIFI_EVENT_AP_STACONNECTED: {
                auto* event = (wifi_event_ap_staconnected_t*)event_data;
                ESP_LOGI(TAG, "Client authenticated via WPA3-SAE on Direct AP: MAC=" MACSTR ", AID=%d", MAC2STR(event->mac), event->aid);
                break;
            }
            case WIFI_EVENT_AP_STADISCONNECTED: {
                auto* event = (wifi_event_ap_stadisconnected_t*)event_data;
                ESP_LOGI(TAG, "Client disconnected from Direct AP: MAC=" MACSTR ", AID=%d", MAC2STR(event->mac), event->aid);
                break;
            }
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
            ESP_LOGI(TAG, "Station connected! Assigned IP: %s", ip);
        }
    }
}

esp_err_t NetworkManager::init(NetworkMode mode) {
    m_mode = mode;
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    m_netif_sta = esp_netif_create_default_wifi_sta();
    m_netif_ap  = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, this, nullptr));

    if (m_mode == NetworkMode::DIRECT_AP) {
        return startDirectAP();
    } else if (m_mode == NetworkMode::STA_ONLY) {
        return startSTA();
    }

    ESP_LOGI(TAG, "Attempting Station connection (Timeout: %d ms)...", STA_CONNECT_TIMEOUT_MS);
    esp_err_t ret = startSTA();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Station start failed, activating Direct SoftAP fallback");
        return startDirectAP();
    }

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                          pdFALSE, pdFALSE,
                                          pdMS_TO_TICKS(STA_CONNECT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Home Network Link established successfully.");
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Home AP unreachable. Elevating to Direct SoftAP (WPA3-SAE / HT40)... ");
        return startDirectAP();
    }
}

esp_err_t NetworkManager::startDirectAP() {
    esp_wifi_stop();

    wifi_config_t ap_config = {};
    strcpy((char*)ap_config.ap.ssid, DIRECT_AP_SSID);
    strcpy((char*)ap_config.ap.password, DIRECT_AP_PASS);
    ap_config.ap.ssid_len = strlen(DIRECT_AP_SSID);
    ap_config.ap.channel = DIRECT_AP_CHANNEL;
    ap_config.ap.authmode = WIFI_AUTH_WPA3_PSK; // Чистый WPA3-Personal (SAE)
    ap_config.ap.max_connection = DIRECT_AP_MAX_CONN;
    ap_config.ap.beacon_interval = 100;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT40));

    m_is_ap = true;
    m_connected = true;
    m_ip_str = "192.168.4.1";
    m_current_ssid = DIRECT_AP_SSID;

    ESP_LOGI(TAG, "===================================================");
    ESP_LOGI(TAG, " SECURE HIGH-SPEED DAC SOFTAP ONLINE (WPA3-SAE)");
    ESP_LOGI(TAG, " SSID:     %s", DIRECT_AP_SSID);
    ESP_LOGI(TAG, " Security: WPA3-Personal (SAE Handshake, PMF Active)");
    ESP_LOGI(TAG, " Password: %s", DIRECT_AP_PASS);
    ESP_LOGI(TAG, " Band:     2.4GHz HT40 (Up to 150-229 Mbps Link)");
    ESP_LOGI(TAG, " Gateway:  http://192.168.4.1");
    ESP_LOGI(TAG, "===================================================");

    return ESP_OK;
}

esp_err_t NetworkManager::startSTA() {
    wifi_config_t sta_config = {};
    strcpy((char*)sta_config.sta.ssid, "HomeAudioNet");
    strcpy((char*)sta_config.sta.password, "audiophile123");
    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT40));

    m_current_ssid = (char*)sta_config.sta.ssid;
    return ESP_OK;
}
