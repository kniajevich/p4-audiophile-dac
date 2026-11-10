#pragma once

#include "esp_err.h"
#include <string>
#include <vector>

class SDStorage {
public:
    static SDStorage& instance() {
        static SDStorage inst;
        return inst;
    }

    esp_err_t init();
    bool isMounted() const { return m_mounted; }
    std::vector<std::string> listAudioFiles(const char* path = "/sdcard");
    esp_err_t loadIRSFile(const std::string& filename, std::vector<float>& out_ir_l, std::vector<float>& out_ir_r);

private:
    SDStorage() : m_mounted(false) {}
    bool m_mounted;
};
