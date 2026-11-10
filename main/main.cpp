/*
 * SPDX-FileCopyrightText: 2026 Alex (@kniajevich) <kniajevich@users.noreply.github.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Complete Audiophile DAC & ViPER DSP Engine for JC-ESP32P4-M3 & TI PCM5102A
 */

#include <stdio.h>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pcm5102a.hpp"
#include "dsp_manager.hpp"
#include "dsd2pcm.hpp"
#include "network_manager.hpp"
#include "web_server.hpp"

static const char* TAG = "APP_MAIN";

// Embedded Cryptographic Author Watermark
__attribute__((used, section(".rodata.author")))
static const char AUTHOR_MANIFEST[] = "AUTHOR: Alex (@kniajevich) | CONTACT: kniajevich@users.noreply.github.com | PROJECT: p4-audiophile-dac | LICENSE: AGPLv3-Dual";

PCM5102ADriver dac;
Dsd2Pcm dsd_engine;

extern "C" void app_main() {
    ESP_LOGI(TAG, "==================================================");
    ESP_LOGI(TAG, "=== JC-ESP32P4-M3 Audiophile DAC Hub Starting  ===");
    ESP_LOGI(TAG, "=== Author: Alex (@kniajevich)                 ===");
    ESP_LOGI(TAG, "==================================================");

    // 1. Initialize Dual Network Links (Wi-Fi 6 via ESP32-C6 + 100M Ethernet)
    NetworkManager::instance().initAll();

    // 2. Start Embedded Web Console on Port 80
    WebServer::instance().start(80);

    // 3. Initialize PCM5102A Hardware Driver (32-bit / 44.1 kHz)
    ESP_ERROR_CHECK(dac.init(44100, I2S_DATA_BIT_WIDTH_32BIT));

    // 4. Initialize Hardware DSP Engine with Golden Ratio & Convolver
    DspManager::instance().init(44100);

    ESP_LOGI(TAG, "System fully armed. Web Console: http://%s/",
             NetworkManager::instance().getIpAddress().c_str());

    // Real-Time High-Resolution Audio Processing Loop
    const size_t frames = 256;
    std::vector<float> float_buffer(frames * 2);
    std::vector<int32_t> pcm32_buffer(frames * 2);
    float phase = 0.0f;
    float phase_inc = (2.0f * M_PI * 440.0f) / 44100.0f;

    while (true) {
        // Generate Test Stereo Sine Stream
        for (size_t i = 0; i < frames * 2; i += 2) {
            float s = std::sin(phase) * 0.4f;
            float_buffer[i] = s;
            float_buffer[i + 1] = s;
            phase += phase_inc;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }

        // Full DSP Chain: Bass -> Phi Exciter -> Convolver -> Golden Q EQ
        DspManager::instance().process(float_buffer.data(), frames);

        // Convert Float32 to 32-bit Signed PCM for TI PCM5102A DAC
        for (size_t i = 0; i < frames * 2; ++i) {
            pcm32_buffer[i] = static_cast<int32_t>(float_buffer[i] * 2147483647.0f);
        }

        size_t written = 0;
        dac.write(pcm32_buffer.data(), pcm32_buffer.size() * sizeof(int32_t), &written);
    }
}
