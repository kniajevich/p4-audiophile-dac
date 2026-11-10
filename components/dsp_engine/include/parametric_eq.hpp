#pragma once

#include <cmath>
#include <cstddef>

struct BiquadCoeffs {
    float b0, b1, b2, a1, a2;
};

class ParametricEQ {
public:
    ParametricEQ() : m_x1_l(0), m_x2_l(0), m_y1_l(0), m_y2_l(0),
                     m_x1_r(0), m_x2_r(0), m_y1_r(0), m_y2_r(0) {
        setPeaking(1000.0f, 0.0f, 1.0f, 44100.0f);
    }

    void setPeaking(float freq, float gain_db, float q, float sample_rate) {
        float a = std::pow(10.0f, gain_db / 40.0f);
        float w0 = 2.0f * M_PI * freq / sample_rate;
        float alpha = std::sin(w0) / (2.0f * q);

        float a0 = 1.0f + alpha / a;
        m_coeffs.b0 = (1.0f + alpha * a) / a0;
        m_coeffs.b1 = (-2.0f * std::cos(w0)) / a0;
        m_coeffs.b2 = (1.0f - alpha * a) / a0;
        m_coeffs.a1 = (-2.0f * std::cos(w0)) / a0;
        m_coeffs.a2 = (1.0f - alpha / a) / a0;
    }

    void processInterleaved(float* buffer, size_t num_frames) {
        for (size_t i = 0; i < num_frames * 2; i += 2) {
            // Left Channel Direct Form II / Transposed
            float in_l = buffer[i];
            float out_l = m_coeffs.b0 * in_l + m_x1_l;
            m_x1_l = m_coeffs.b1 * in_l - m_coeffs.a1 * out_l + m_x2_l;
            m_x2_l = m_coeffs.b2 * in_l - m_coeffs.a2 * out_l;
            buffer[i] = out_l;

            // Right Channel
            float in_r = buffer[i + 1];
            float out_r = m_coeffs.b0 * in_r + m_x1_r;
            m_x1_r = m_coeffs.b1 * in_r - m_coeffs.a1 * out_r + m_x2_r;
            m_x2_r = m_coeffs.b2 * in_r - m_coeffs.a2 * out_r;
            buffer[i + 1] = out_r;
        }
    }

private:
    BiquadCoeffs m_coeffs;
    float m_x1_l, m_x2_l, m_y1_l, m_y2_l;
    float m_x1_r, m_x2_r, m_y1_r, m_y2_r;
};
