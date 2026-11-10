#pragma once

#include <cstdint>
#include "esp_err.h"
#include "driver/i2s_std.h"

class PCM5102ADriver {
public:
    PCM5102ADriver() : m_tx_handle(nullptr) {}

    esp_err_t init(uint32_t sample_rate = 44100, i2s_data_bit_width_t bits_per_sample = I2S_DATA_BIT_WIDTH_32BIT);
    esp_err_t write(const void* src, size_t size, size_t* bytes_written, uint32_t timeout_ms = 100);
    esp_err_t setSampleRate(uint32_t sample_rate);

private:
    i2s_chan_handle_t m_tx_handle;
};
