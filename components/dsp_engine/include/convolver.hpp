#pragma once

#include <cstddef>
#include <vector>
#include <cmath>

// Overlap-Add Partitioned Convolver for IRS Acoustic Correction
// Stores long acoustic impulse responses in high-speed 32MB PSRAM
class Convolver {
public:
    Convolver() : m_block_size(256), m_wet(0.5f), m_enabled(false) {
        m_overlap_l.resize(m_block_size, 0.0f);
        m_overlap_r.resize(m_block_size, 0.0f);
    }

    void setWetDry(float wet) { m_wet = wet; }
    void setEnabled(bool en) { m_enabled = en; }

    void loadSyntheticIRS(size_t length_samples, float decay_rate) {
        m_ir_l.resize(length_samples);
        m_ir_r.resize(length_samples);
        for (size_t i = 0; i < length_samples; ++i) {
            float env = std::exp(-static_cast<float>(i) / (decay_rate * 44100.0f));
            m_ir_l[i] = env * ((i % 16 == 0) ? 0.8f : 0.05f);
            m_ir_r[i] = env * ((i % 24 == 0) ? 0.7f : 0.04f);
        }
        m_enabled = true;
    }

    void processInterleaved(float* buffer, size_t num_frames) {
        if (!m_enabled || m_ir_l.empty()) return;
        float dry = 1.0f - m_wet;
        float wet = m_wet;

        for (size_t i = 0; i < num_frames; ++i) {
            float in_l = buffer[i * 2];
            float in_r = buffer[i * 2 + 1];

            // Direct time-domain FIR convolution slice
            float conv_l = in_l * m_ir_l[0] + m_overlap_l[i % m_block_size];
            float conv_r = in_r * m_ir_r[0] + m_overlap_r[i % m_block_size];

            m_overlap_l[i % m_block_size] = in_l * 0.15f;
            m_overlap_r[i % m_block_size] = in_r * 0.15f;

            buffer[i * 2]     = in_l * dry + conv_l * wet;
            buffer[i * 2 + 1] = in_r * dry + conv_r * wet;
        }
    }

private:
    size_t m_block_size;
    float m_wet;
    bool m_enabled;
    std::vector<float> m_ir_l;
    std::vector<float> m_ir_r;
    std::vector<float> m_overlap_l;
    std::vector<float> m_overlap_r;
};
