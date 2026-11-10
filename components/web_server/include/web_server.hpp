#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

class WebServer {
public:
    static WebServer& instance() {
        static WebServer inst;
        return inst;
    }

    esp_err_t start(uint16_t port = 80);
    void stop();

private:
    WebServer() : m_server(nullptr) {}
    httpd_handle_t m_server;
};
