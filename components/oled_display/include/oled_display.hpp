#pragma once

#include "esp_err.h"
#include <cstdint>

class OledDisplay {
public:
    static OledDisplay& instance() {
        static OledDisplay inst;
        return inst;
    }

    esp_err_t init(int sda_pin = 7, int scl_pin = 8);
    void updateStatus(uint32_t sample_rate, const char* source, float vu_left, float vu_right);

private:
    OledDisplay() : m_initialized(false) {}
    bool m_initialized;
};
