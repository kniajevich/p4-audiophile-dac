#pragma once

#include "dynamic_bass.hpp"
#include "parametric_eq.hpp"
#include "phi_dsp.hpp"
#include "convolver.hpp"
#include <atomic>

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

    void loadIRS(const std::vector<float>& l, const std::vector<float>& r) {
        m_convolver.loadCustomIRS(l, r);
    }

    DspConfig getConfig() const { return m_cfg; }
    float getVuL() const { return m_vu_l.load(); }
    float getVuR() const { return m_vu_r.load(); }

    void process(float* buffer, size_t num_frames) {
        if (m_cfg.bass_enabled) m_bass.processInterleaved(buffer, num_frames, m_sample_rate);
        if (m_cfg.phi_exciter_enabled) m_exciter.processInterleaved(buffer, num_frames);
        if (m_cfg.convolver_enabled) m_convolver.processInterleaved(buffer, num_frames);
        if (m_cfg.eq_enabled) m_eq.processInterleaved(buffer, num_frames);

        m_watermark.processInterleaved(buffer, num_frames);

        float vol = m_cfg.master_volume;
        float peak_l = 0.0f, peak_r = 0.0f;
        for (size_t i = 0; i < num_frames * 2; i += 2) {
            buffer[i] *= vol;
            buffer[i + 1] *= vol;
            float al = std::fabs(buffer[i]);
            float ar = std::fabs(buffer[i + 1]);
            if (al > peak_l) peak_l = al;
            if (ar > peak_r) peak_r = ar;
        }
        m_vu_l.store(peak_l);
        m_vu_r.store(peak_r);
    }

private:
    DspManager() : m_vu_l(0.0f), m_vu_r(0.0f) {}
    DspConfig m_cfg;
    DynamicBass m_bass;
    PhiHarmonicExciter m_exciter;
    Convolver m_convolver;
    ParametricEQ m_eq;
    SupersonicWatermark m_watermark;
    uint32_t m_sample_rate = 44100;
    std::atomic<float> m_vu_l;
    std::atomic<float> m_vu_r;
};
