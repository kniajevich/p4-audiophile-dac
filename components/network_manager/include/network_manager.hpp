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
    inline esp_err_t initAll() { return init(NetworkMode::AUTO_FALLBACK); }

    esp_err_t startDirectAP();
    esp_err_t startSTA();
    esp_err_t setCredentials(const std::string& ssid, const std::string& password);
    esp_err_t setAPCredentials(const std::string& ssid, const std::string& password);

    bool isConnected() const { return m_connected; }
    bool isDirectAP() const { return m_is_ap; }

    std::string getIPAddress() const { return m_ip_str; }
    std::string getIpAddress() const { return m_ip_str; }

    std::string getSSID() const { return m_current_ssid; }
    std::string getAPSSID() const { return m_ap_ssid; }

private:
    NetworkManager() = default;
    ~NetworkManager() = default;

    void loadCredentials();
    void saveCredentials(const std::string& ssid, const std::string& password);
    void saveAPCredentials(const std::string& ssid, const std::string& password);

    bool m_connected = false;
    bool m_is_ap = true;
    NetworkMode m_mode = NetworkMode::AUTO_FALLBACK;
    std::string m_ip_str = "192.168.4.1";
    std::string m_current_ssid = "JC-P4-DirectDAC";
    std::string m_sta_ssid = "";
    std::string m_sta_pass = "";
    std::string m_ap_ssid = "JC-P4-DirectDAC";
    std::string m_ap_pass = "audiophile123";
};
