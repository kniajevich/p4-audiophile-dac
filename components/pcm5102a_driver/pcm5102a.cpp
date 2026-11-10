#include "pcm5102a.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char* TAG = "PCM5102A";

#define I2S_BCK_IO      GPIO_NUM_12
#define I2S_WS_IO       GPIO_NUM_13
#define I2S_DO_IO       GPIO_NUM_11

esp_err_t PCM5102ADriver::init(uint32_t sample_rate, i2s_data_bit_width_t bits_per_sample) {
    ESP_LOGI(TAG, "Initializing PCM5102A I2S Master driver @ %lu Hz...", sample_rate);

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 256;

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &m_tx_handle, nullptr));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bits_per_sample, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCK_IO,
            .ws   = I2S_WS_IO,
            .dout = I2S_DO_IO,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(m_tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(m_tx_handle));
    ESP_LOGI(TAG, "PCM5102A I2S driver running.");
    return ESP_OK;
}

esp_err_t PCM5102ADriver::write(const void* src, size_t size, size_t* bytes_written, uint32_t timeout_ms) {
    return i2s_channel_write(m_tx_handle, src, size, bytes_written, pdMS_TO_TICKS(timeout_ms));
}

esp_err_t PCM5102ADriver::setSampleRate(uint32_t sample_rate) {
    if (!m_tx_handle) return ESP_ERR_INVALID_STATE;
    i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate);
    return i2s_channel_reconfig_std_clock(m_tx_handle, &clk_cfg);
}
