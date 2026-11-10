#pragma once

#include "dynamic_bass.hpp"
#include "parametric_eq.hpp"
#include <atomic>

struct DspConfig {
    float master_volume = 0.85f;
    bool bass_enabled = true;
    float bass_intensity = 0.6f;
    float bass_cutoff = 75.0f;
    bool eq_enabled = true;
    float eq_low_gain = 2.0f;
    float eq_mid_gain = 0.0f;
    float eq_high_gain = 1.5f;
    int active_source = 0; // 0: Web Stream, 1: Bluetooth, 2: SD Card, 3: Ethernet
};

class DspManager {
public:
    static DspManager& instance() {
        static DspManager inst;
        return inst;
    }

    void init(uint32_t sample_rate = 44100) {
        m_sample_rate = sample_rate;
        m_bass.setIntensity(m_cfg.bass_intensity);
        m_eq.setPeaking(1000.0f, m_cfg.eq_mid_gain, 1.414f, sample_rate);
    }

    void updateConfig(const DspConfig& cfg) {
        m_cfg = cfg;
        m_bass.setIntensity(cfg.bass_enabled ? cfg.bass_intensity : 0.0f);
    }

    DspConfig getConfig() const { return m_cfg; }

    void process(float* buffer, size_t num_frames) {
        if (m_cfg.bass_enabled) {
            m_bass.processInterleaved(buffer, num_frames, m_sample_rate);
        }
        if (m_cfg.eq_enabled) {
            m_eq.processInterleaved(buffer, num_frames);
        }
        float vol = m_cfg.master_volume;
        for (size_t i = 0; i < num_frames * 2; ++i) {
            buffer[i] *= vol;
        }
    }

private:
    DspManager() = default;
    DspConfig m_cfg;
    DynamicBass m_bass;
    ParametricEQ m_eq;
    uint32_t m_sample_rate = 44100;
};
