#pragma once

#include <cmath>
#include <cstddef>

// Golden Ratio Mathematical Constant
constexpr float PHI = 1.618033988749895f;
constexpr float INV_PHI = 0.618033988749895f;
constexpr float GOLDEN_Q = 0.786151377757423f; // sqrt(1 / PHI) - Minimal phase ringing

// Golden Ratio Harmonic Tube Exciter
// Generates progressive Fibonacci harmonic overtones: H2 (1/phi^2), H3 (1/phi^3)
class PhiHarmonicExciter {
public:
    PhiHarmonicExciter() : m_amount(0.4f) {}

    void setAmount(float amount) { m_amount = amount; }

    void processInterleaved(float* buffer, size_t num_frames) {
        if (m_amount <= 0.001f) return;
        const float w2 = (1.0f / (PHI * PHI)) * m_amount; // 2nd harmonic weight (~0.382)
        const float w3 = (1.0f / (PHI * PHI * PHI)) * m_amount; // 3rd harmonic weight (~0.236)

        for (size_t i = 0; i < num_frames * 2; ++i) {
            float x = buffer[i];
            float x2 = x * x;
            float x3 = x2 * x;
            // Harmonic polynomial expansion
            float saturated = x + (w2 * x2) - (w3 * x3);
            // Soft saturation ceiling
            buffer[i] = std::tanh(saturated);
        }
    }

private:
    float m_amount;
};
