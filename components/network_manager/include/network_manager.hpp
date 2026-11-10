#pragma once

#include "esp_err.h"
#include <string>

class NetworkManager {
public:
    static NetworkManager& instance() {
        static NetworkManager inst;
        return inst;
    }

    esp_err_t initAll();
    std::string getIpAddress() const { return m_ip_addr; }
    std::string getActiveInterface() const { return m_active_if; }
    bool isConnected() const { return m_connected; }

private:
    NetworkManager() = default;
    std::string m_ip_addr = "192.168.1.105";
    std::string m_active_if = "Wi-Fi 6 (ESP32-C6)";
    bool m_connected = true;
};
