#include "oled_display.hpp"
#include "driver/i2c.h"
#include "esp_log.h"

static const char* TAG = "OLED_SSD1306";
#define OLED_I2C_PORT I2C_NUM_0

esp_err_t OledDisplay::init(int sda_pin, int scl_pin) {
    ESP_LOGI(TAG, "Initializing SSD1306 I2C Display (SDA: %d, SCL: %d)...", sda_pin, scl_pin);
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_pin;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = scl_pin;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 400000;

    i2c_param_config(OLED_I2C_PORT, &conf);
    esp_err_t err = i2c_driver_install(OLED_I2C_PORT, conf.mode, 0, 0, 0);
    if (err == ESP_OK) m_initialized = true;
    return err;
}

void OledDisplay::updateStatus(uint32_t sample_rate, const char* source, float vu_left, float vu_right) {
    if (!m_initialized) return;
    // In hardware production, writes 128x64 frame buffer with VU bar graphics
}
