#pragma once

#include "esp_err.h"
#include "esp_ota_ops.h"

class OTAManager {
public:
    static OTAManager& instance() {
        static OTAManager inst;
        return inst;
    }

    esp_err_t begin();
    esp_err_t writeChunk(const void* data, size_t length);
    esp_err_t endAndReboot();
    void abort();

private:
    OTAManager() : m_handle(0), m_update_partition(nullptr), m_in_progress(false) {}
    esp_ota_handle_t m_handle;
    const esp_partition_t* m_update_partition;
    bool m_in_progress;
};
