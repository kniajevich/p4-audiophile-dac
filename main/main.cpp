#include <stdio.h>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pcm5102a.hpp"
#include "dynamic_bass.hpp"
#include "parametric_eq.hpp"

static const char* TAG = "APP_MAIN";

PCM5102ADriver dac;
DynamicBass dynamic_bass;
ParametricEQ eq_1khz;

extern "C" void app_main() {
    ESP_LOGI(TAG, "=== ESP32-P4 Audiophile Network DAC starting ===");

    // 1. Initialize PCM5102A I2S Driver at 32-bit / 44.1 kHz
    ESP_ERROR_CHECK(dac.init(44100, I2S_DATA_BIT_WIDTH_32BIT));

    // 2. Configure ViPER-style DSP pipeline
    dynamic_bass.setIntensity(0.6f); // 60% harmonic boost
    eq_1khz.setPeaking(1000.0f, 3.0f, 1.414f, 44100.0f); // +3dB acoustic presence

    ESP_LOGI(TAG, "DSP Pipeline & PCM5102A DAC initialized successfully.");

    // Test Tone Generator (440 Hz Sine)
    const size_t frames = 256;
    std::vector<float> float_buffer(frames * 2);
    std::vector<int32_t> pcm32_buffer(frames * 2);
    float phase = 0.0f;
    float phase_inc = (2.0f * M_PI * 440.0f) / 44100.0f;

    while (true) {
        // Generate Sine Stereo stream
        for (size_t i = 0; i < frames * 2; i += 2) {
            float sample = std::sin(phase) * 0.4f;
            float_buffer[i] = sample;
            float_buffer[i + 1] = sample;
            phase += phase_inc;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }

        // Run ViPER DSP Chain
        dynamic_bass.processInterleaved(float_buffer.data(), frames, 44100.0f);
        eq_1khz.processInterleaved(float_buffer.data(), frames);

        // Convert Float32 (-1.0..1.0) to 32-bit Signed PCM for PCM5102A
        for (size_t i = 0; i < frames * 2; ++i) {
            pcm32_buffer[i] = static_cast<int32_t>(float_buffer[i] * 2147483647.0f);
        }

        size_t written = 0;
        dac.write(pcm32_buffer.data(), pcm32_buffer.size() * sizeof(int32_t), &written);
    }
}
