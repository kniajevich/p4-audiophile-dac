#pragma once

#include "dynamic_bass.hpp"
#include "parametric_eq.hpp"
#include "phi_dsp.hpp"
#include "convolver.hpp"

struct DspConfig {
    float master_volume = 0.85f;
    bool bass_enabled = true;
    float bass_intensity = 0.6f;
    bool phi_exciter_enabled = true;
    float phi_amount = 0.45f;
    bool convolver_enabled = true;
    float convolver_wet = 0.35f;
    bool eq_enabled = true;
    float eq_low_gain = 2.0f;
    float eq_high_gain = 1.5f;
    int active_source = 0;
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
        m_exciter.setAmount(m_cfg.phi_amount);
        m_convolver.loadSyntheticIRS(2048, 0.05f);
        m_convolver.setWetDry(m_cfg.convolver_wet);
        m_convolver.setEnabled(m_cfg.convolver_enabled);
        m_eq.setPeaking(1000.0f, 2.0f, GOLDEN_Q, sample_rate);
        m_watermark.init(sample_rate);
    }

    void updateConfig(const DspConfig& cfg) {
        m_cfg = cfg;
        m_bass.setIntensity(cfg.bass_enabled ? cfg.bass_intensity : 0.0f);
        m_exciter.setAmount(cfg.phi_exciter_enabled ? cfg.phi_amount : 0.0f);
        m_convolver.setWetDry(cfg.convolver_wet);
        m_convolver.setEnabled(cfg.convolver_enabled);
    }

    DspConfig getConfig() const { return m_cfg; }

    void process(float* buffer, size_t num_frames) {
        if (m_cfg.bass_enabled) m_bass.processInterleaved(buffer, num_frames, m_sample_rate);
        if (m_cfg.phi_exciter_enabled) m_exciter.processInterleaved(buffer, num_frames);
        if (m_cfg.convolver_enabled) m_convolver.processInterleaved(buffer, num_frames);
        if (m_cfg.eq_enabled) m_eq.processInterleaved(buffer, num_frames);

        // Inaudible Supersonic Signature injection
        m_watermark.processInterleaved(buffer, num_frames);

        float vol = m_cfg.master_volume;
        for (size_t i = 0; i < num_frames * 2; ++i) buffer[i] *= vol;
    }

private:
    DspManager() = default;
    DspConfig m_cfg;
    DynamicBass m_bass;
    PhiHarmonicExciter m_exciter;
    Convolver m_convolver;
    ParametricEQ m_eq;
    SupersonicWatermark m_watermark;
    uint32_t m_sample_rate = 44100;
};
