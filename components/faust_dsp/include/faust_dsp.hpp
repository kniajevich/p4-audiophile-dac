#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>

class FaustDSP {
public:
    static FaustDSP& instance() {
        static FaustDSP s_instance;
        return s_instance;
    }

    void init(uint32_t sample_rate);
    void process(const float* in_l, const float* in_r, float* out_l, float* out_r, size_t frames);

    // Tube controls
    void setDrive(float drive) { m_target_drive.store(drive, std::memory_order_relaxed); }
    void setBias(float bias) { m_target_bias.store(bias, std::memory_order_relaxed); }
    void setMix(float mix) { m_target_mix.store(mix, std::memory_order_relaxed); }
    void setBypass(bool bypass) { m_bypass.store(bypass, std::memory_order_relaxed); }

    // Phi-EQ Band Controls (+-6.0 dB)
    void setGainSub(float db)      { m_target_g_sub.store(db, std::memory_order_relaxed); }
    void setGainWarmth(float db)   { m_target_g_warmth.store(db, std::memory_order_relaxed); }
    void setGainPresence(float db) { m_target_g_presence.store(db, std::memory_order_relaxed); }
    void setGainAir(float db)      { m_target_g_air.store(db, std::memory_order_relaxed); }

    float getDrive() const { return m_target_drive.load(std::memory_order_relaxed); }
    float getBias() const  { return m_target_bias.load(std::memory_order_relaxed); }
    float getMix() const   { return m_target_mix.load(std::memory_order_relaxed); }
    bool isBypassed() const { return m_bypass.load(std::memory_order_relaxed); }

private:
    FaustDSP();
    ~FaustDSP() = default;

    // Рациональная аппроксимация Паде [3/2] для тангенса
    static inline float fast_tanh(float x) {
        if (x <= -3.0f) return -1.0f;
        if (x >= 3.0f)  return 1.0f;
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    struct Biquad {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float s1_l = 0.0f, s2_l = 0.0f;
        float s1_r = 0.0f, s2_r = 0.0f;

        inline void updatePeak(float f0, float gain_db, float q, float fs) {
            const float a = std::pow(10.0f, gain_db / 40.0f);
            const float w0 = 2.0f * 3.141592653589793f * f0 / fs;
            const float alpha = std::sin(w0) / (2.0f * q);
            const float cos_w0 = std::cos(w0);

            const float a0 = 1.0f + alpha / a;
            b0 = (1.0f + alpha * a) / a0;
            b1 = (-2.0f * cos_w0) / a0;
            b2 = (1.0f - alpha * a) / a0;
            a1 = (-2.0f * cos_w0) / a0;
            a2 = (1.0f - alpha / a) / a0;
        }

        inline void process(float in_l, float in_r, float& out_l, float& out_r) {
            // DF2T Left
            out_l = b0 * in_l + s1_l;
            s1_l = b1 * in_l - a1 * out_l + s2_l;
            s2_l = b2 * in_l - a2 * out_l;
            // DF2T Right
            out_r = b0 * in_r + s1_r;
            s1_r = b1 * in_r - a1 * out_r + s2_r;
            s2_r = b2 * in_r - a2 * out_r;
        }
    };

    uint32_t m_sample_rate = 48000;

    // DC Blocker states
    float m_dc_x_l = 0.0f, m_dc_y_l = 0.0f;
    float m_dc_x_r = 0.0f, m_dc_y_r = 0.0f;

    // 4 Fibonacci Bands: 55Hz, 377Hz, 2584Hz, 6765Hz
    Biquad m_eq_sub;
    Biquad m_eq_warmth;
    Biquad m_eq_presence;
    Biquad m_eq_air;

    // Smoothed k-rate parameters
    float m_cur_drive = 1.2f;
    float m_cur_bias = 0.18f;
    float m_cur_mix = 0.5f;
    float m_cur_g_sub = 0.0f;
    float m_cur_g_warmth = 0.0f;
    float m_cur_g_presence = 0.0f;
    float m_cur_g_air = 0.0f;

    std::atomic<float> m_target_drive{1.2f};
    std::atomic<float> m_target_bias{0.18f};
    std::atomic<float> m_target_mix{0.5f};
    std::atomic<float> m_target_g_sub{0.0f};
    std::atomic<float> m_target_g_warmth{0.0f};
    std::atomic<float> m_target_g_presence{0.0f};
    std::atomic<float> m_target_g_air{0.0f};
    std::atomic<bool>  m_bypass{false};
};
