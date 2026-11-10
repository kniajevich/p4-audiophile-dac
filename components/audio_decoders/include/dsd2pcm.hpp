#pragma once

#include <cstdint>
#include <cstddef>

// Fast DSD64 / DSD128 1-bit PDM to Multi-bit PCM Decimator
class Dsd2Pcm {
public:
    Dsd2Pcm() : m_filter_state(0) {}

    // Decimates 8 bits of 1-bit DSD stream into single 32-bit float PCM sample
    inline float decimateByte(uint8_t dsd_byte) {
        int count = 0;
        for (int i = 0; i < 8; ++i) {
            count += (dsd_byte & (1 << i)) ? 1 : -1;
        }
        float target = static_cast<float>(count) / 8.0f;
        m_filter_state += 0.125f * (target - m_filter_state); // Sinc-1 averaging
        return m_filter_state;
    }

private:
    float m_filter_state;
};
