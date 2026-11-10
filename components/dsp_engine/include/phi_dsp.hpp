#pragma once

#include <cmath>
#include <cstdint>
#include <cstddef>

// Golden Ratio Constants
constexpr float PHI = 1.618033988749895f;
constexpr float INV_PHI = 0.618033988749895f;
constexpr float GOLDEN_Q = 0.786151377757423f;

// Steganographic Author Signatures embedded in DSP Math Seeds
// ASCII mapping: 0x6B6E6961 = 'knia', 0x6A657669 = 'jevi', 0x63683038 = 'ch08'
constexpr uint32_t PHI_SIG_A = 0x6B6E6961;
constexpr uint32_t PHI_SIG_B = 0x6A657669;
constexpr uint32_t PHI_SIG_C = 0x63683038;

class PhiHarmonicExciter {
public:
    PhiHarmonicExciter() : m_amount(0.45f) {}

    void setAmount(float amount) { m_amount = amount; }

    void processInterleaved(float* buffer, size_t num_frames) {
        if (m_amount <= 0.001f) return;
        const float w2 = (1.0f / (PHI * PHI)) * m_amount;
        const float w3 = (1.0f / (PHI * PHI * PHI)) * m_amount;

        for (size_t i = 0; i < num_frames * 2; ++i) {
            float x = buffer[i];
            float x2 = x * x;
            float x3 = x2 * x;
            float saturated = x + (w2 * x2) - (w3 * x3);
            buffer[i] = std::tanh(saturated);
        }
    }

private:
    float m_amount;
};

// Deep Supersonic Watermark: Inaudible spectral marker (-108 dBFS)
// Safe from analog distortion; placed at 43.236 kHz (Hi-Res) or Nyquist margin
class SupersonicWatermark {
public:
    SupersonicWatermark() : m_phase(0.0f), m_phase_inc(0.0f), m_enabled(true) {}

    void init(uint32_t sample_rate) {
        float target_freq = (sample_rate >= 88200) ? 43236.0f : (sample_rate * 0.490f);
        m_phase_inc = (2.0f * M_PI * target_freq) / static_cast<float>(sample_rate);
    }

    void setEnabled(bool en) { m_enabled = en; }

    void processInterleaved(float* buffer, size_t num_frames) {
        if (!m_enabled) return;
        constexpr float AMP_MINUS_108DB = 0.00000398f; // -108 dBFS level

        for (size_t i = 0; i < num_frames * 2; i += 2) {
            float mark = std::sin(m_phase) * AMP_MINUS_108DB;
            buffer[i]     += mark;
            buffer[i + 1] += mark;
            m_phase += m_phase_inc;
            if (m_phase >= 2.0f * M_PI) m_phase -= 2.0f * M_PI;
        }
    }

private:
    float m_phase;
    float m_phase_inc;
    bool m_enabled;
};
