#pragma once

#include <cmath>
#include <cstddef>

// ViPER4Android-style Dynamic Bass Synthesizer
// Extracts sub-bass band (30-80 Hz) and generates 2nd/3rd harmonics via soft-saturation
class DynamicBass {
public:
    DynamicBass() : m_cutoff(75.0f), m_intensity(0.5f), m_lp_state_l(0), m_lp_state_r(0) {}

    void setIntensity(float intensity) { m_intensity = intensity; }

    void processInterleaved(float* buffer, size_t num_frames, float sample_rate) {
        if (m_intensity <= 0.001f) return;
        float rc = 1.0f / (2.0f * M_PI * m_cutoff);
        float dt = 1.0f / sample_rate;
        float alpha = dt / (rc + dt);

        for (size_t i = 0; i < num_frames * 2; i += 2) {
            // 1-pole Lowpass extraction for bass fundamental
            m_lp_state_l += alpha * (buffer[i] - m_lp_state_l);
            m_lp_state_r += alpha * (buffer[i + 1] - m_lp_state_r);

            // Non-linear harmonic exciter: f(x) = tanh(k*x)
            float bass_h_l = std::tanh(m_lp_state_l * 2.5f);
            float bass_h_r = std::tanh(m_lp_state_r * 2.5f);

            buffer[i]     += bass_h_l * m_intensity;
            buffer[i + 1] += bass_h_r * m_intensity;
        }
    }

private:
    float m_cutoff;
    float m_intensity;
    float m_lp_state_l;
    float m_lp_state_r;
};
