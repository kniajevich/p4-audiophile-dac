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

    // Потокобезопасная установка целевых параметров
    void setDrive(float drive) { m_target_drive.store(drive, std::memory_order_relaxed); }
    void setBias(float bias) { m_target_bias.store(bias, std::memory_order_relaxed); }
    void setWarmth(float hz) { m_target_warmth.store(hz, std::memory_order_relaxed); }
    void setMix(float mix) { m_target_mix.store(mix, std::memory_order_relaxed); }
    void setBypass(bool bypass) { m_bypass.store(bypass, std::memory_order_relaxed); }

    float getDrive() const { return m_target_drive.load(std::memory_order_relaxed); }
    float getBias() const { return m_target_bias.load(std::memory_order_relaxed); }
    float getWarmth() const { return m_target_warmth.load(std::memory_order_relaxed); }
    float getMix() const { return m_target_mix.load(std::memory_order_relaxed); }
    bool isBypassed() const { return m_bypass.load(std::memory_order_relaxed); }

private:
    FaustDSP();
    ~FaustDSP() = default;

    uint32_t m_sample_rate = 48000;

    // Состояния фильтра DC-blocker (15 Гц, межкаскадный конденсатор)
    float m_dc_x_l = 0.0f;
    float m_dc_y_l = 0.0f;
    float m_dc_x_r = 0.0f;
    float m_dc_y_r = 0.0f;

    // Состояния выходного сглаживающего ФНЧ
    float m_post_lpf_l = 0.0f;
    float m_post_lpf_r = 0.0f;

    // Текущие сглаженные параметры (si.smoo)
    float m_cur_drive = 1.2f;
    float m_cur_bias = 0.18f;
    float m_cur_warmth = 16000.0f;
    float m_cur_mix = 0.5f;

    // Целевые атомарные параметры из веб-интерфейса
    std::atomic<float> m_target_drive{1.2f};
    std::atomic<float> m_target_bias{0.18f};
    std::atomic<float> m_target_warmth{16000.0f};
    std::atomic<float> m_target_mix{0.5f};
    std::atomic<bool>  m_bypass{false};
};
