#pragma once

#include "esp_err.h"
#include "esp_netif.h"
#include <string>

enum class NetworkMode {
    AUTO_FALLBACK,
    DIRECT_AP,
    STA_ONLY
};

class NetworkManager {
public:
    static NetworkManager& instance() {
        static NetworkManager s_instance;
        return s_instance;
    }

    esp_err_t init(NetworkMode mode = NetworkMode::AUTO_FALLBACK);
    esp_err_t startDirectAP();
    esp_err_t startSTA();
    esp_err_t setCredentials(const std::string& ssid, const std::string& password);
    esp_err_t setAPCredentials(const std::string& ssid, const std::string& password);

    bool isConnected() const { return m_connected; }
    bool isDirectAP() const { return m_is_ap; }
    
    // Оба варианта для полной совместимости с main.cpp и вызовами
    std::string getIPAddress() const { return m_ip_str; }
    std::string getIpAddress() const { return m_ip_str; }
    
    std::string getSSID() const { return m_current_ssid; }
    std::string getAPSSID() const { return m_ap_ssid; }

private:
    NetworkManager() = default;
    ~NetworkManager() = default;

    static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
    void loadCredentials();
    void saveCredentials(const std::string& ssid, const std::string& password);
    void saveAPCredentials(const std::string& ssid, const std::string& password);

    bool m_connected = false;
    bool m_is_ap = false;
    NetworkMode m_mode = NetworkMode::AUTO_FALLBACK;
    std::string m_ip_str = "0.0.0.0";
    std::string m_current_ssid = "";
    std::string m_sta_ssid = "";
    std::string m_sta_pass = "";
    std::string m_ap_ssid = "JC-P4-DirectDAC";
    std::string m_ap_pass = "audiophile123";

    esp_netif_t* m_netif_sta = nullptr;
    esp_netif_t* m_netif_ap = nullptr;
};
