#include "faust_dsp.hpp"
#include <cmath>
#include <algorithm>

FaustDSP::FaustDSP() {}

void FaustDSP::init(uint32_t sample_rate) {
    m_sample_rate = sample_rate ? sample_rate : 48000;
    m_dc_x_l = m_dc_y_l = 0.0f;
    m_dc_x_r = m_dc_y_r = 0.0f;
    m_post_lpf_l = m_post_lpf_r = 0.0f;

    m_cur_drive = m_target_drive.load(std::memory_order_relaxed);
    m_cur_bias = m_target_bias.load(std::memory_order_relaxed);
    m_cur_warmth = m_target_warmth.load(std::memory_order_relaxed);
    m_cur_mix = m_target_mix.load(std::memory_order_relaxed);
}

void FaustDSP::process(const float* in_l, const float* in_r, float* out_l, float* out_r, size_t frames) {
    if (m_bypass.load(std::memory_order_relaxed)) {
        std::copy(in_l, in_l + frames, out_l);
        std::copy(in_r, in_r + frames, out_r);
        return;
    }

    const float target_drive  = m_target_drive.load(std::memory_order_relaxed);
    const float target_bias   = m_target_bias.load(std::memory_order_relaxed);
    const float target_warmth = m_target_warmth.load(std::memory_order_relaxed);
    const float target_mix    = m_target_mix.load(std::memory_order_relaxed);

    // k-rate: Сглаживание параметров пропорционально размеру блока сэмплов
    const float block_smoo = 1.0f - std::exp(-static_cast<float>(frames) / (0.025f * static_cast<float>(m_sample_rate)));
    m_cur_drive  += block_smoo * (target_drive  - m_cur_drive);
    m_cur_bias   += block_smoo * (target_bias   - m_cur_bias);
    m_cur_warmth += block_smoo * (target_warmth - m_cur_warmth);
    m_cur_mix    += block_smoo * (target_mix    - m_cur_mix);

    // k-rate: Вынос всех тяжёлых расчётов нормализации и фильтров за пределы цикла
    const float cur_drive = m_cur_drive;
    const float cur_bias  = m_cur_bias;
    const float cur_mix   = m_cur_mix;
    const float cur_dry   = 1.0f - cur_mix;

    const float tanh_bias = fast_tanh(cur_bias);
    const float max_neg_swing = std::abs(fast_tanh(-1.0f * cur_drive + cur_bias) - tanh_bias) + 1e-6f;
    const float inv_neg_swing = 1.0f / max_neg_swing;

    // k-rate: Расчёт констант фильтров
    const float dt = 1.0f / static_cast<float>(m_sample_rate);
    const float rc = 1.0f / (2.0f * 3.141592653589793f * m_cur_warmth);
    const float alpha_lpf = dt / (rc + dt);
    const float r_dc = 1.0f - (2.0f * 3.141592653589793f * 15.0f * dt);

    // a-rate: Внутренний оптимизированный аудиоцикл (только умножения, сложения и fast_tanh)
    for (size_t i = 0; i < frames; ++i) {
        // Левый канал
        const float xl = in_l[i];
        const float in_sat_l = fast_tanh(xl * cur_drive);
        const float v_plate_l = fast_tanh(in_sat_l + cur_bias) - tanh_bias;
        const float norm_l = v_plate_l * inv_neg_swing;

        const float safe_tube_l = norm_l - m_dc_x_l + r_dc * m_dc_y_l;
        m_dc_x_l = norm_l;
        m_dc_y_l = safe_tube_l;

        const float dry_wet_l = xl * cur_dry + safe_tube_l * cur_mix;
        m_post_lpf_l += alpha_lpf * (dry_wet_l - m_post_lpf_l);
        out_l[i] = m_post_lpf_l;

        // Правый канал
        const float xr = in_r[i];
        const float in_sat_r = fast_tanh(xr * cur_drive);
        const float v_plate_r = fast_tanh(in_sat_r + cur_bias) - tanh_bias;
        const float norm_r = v_plate_r * inv_neg_swing;

        const float safe_tube_r = norm_r - m_dc_x_r + r_dc * m_dc_y_r;
        m_dc_x_r = norm_r;
        m_dc_y_r = safe_tube_r;

        const float dry_wet_r = xr * cur_dry + safe_tube_r * cur_mix;
        m_post_lpf_r += alpha_lpf * (dry_wet_r - m_post_lpf_r);
        out_r[i] = m_post_lpf_r;
    }
}
