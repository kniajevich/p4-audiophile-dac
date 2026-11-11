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

    // si.smoo коэффициент сглаживания (эквивалент ~25 мс для подавления zipper noise)
    const float smoo_alpha = 0.0025f;

    // Коэффициент DC-blocker на 15.0 Гц: R = 1 - (2 * pi * fc / fs)
    const float r_dc = 1.0f - (2.0f * 3.141592653589793f * 15.0f / static_cast<float>(m_sample_rate));
    const float dt = 1.0f / static_cast<float>(m_sample_rate);

    for (size_t i = 0; i < frames; ++i) {
        // Посэмпловое сглаживание параметров (si.smoo)
        m_cur_drive  += smoo_alpha * (target_drive  - m_cur_drive);
        m_cur_bias   += smoo_alpha * (target_bias   - m_cur_bias);
        m_cur_warmth += smoo_alpha * (target_warmth - m_cur_warmth);
        m_cur_mix    += smoo_alpha * (target_mix    - m_cur_mix);

        const float tanh_bias = std::tanh(m_cur_bias);
        // Автобалансировка максимального размаха: исключает клиппинг отрицательной полуволны
        const float max_neg_swing = std::abs(std::tanh(-1.0f * m_cur_drive + m_cur_bias) - tanh_bias) + 1e-6f;
        const float inv_neg_swing = 1.0f / max_neg_swing;

        // Коэффициент выходного ФНЧ каскада
        const float rc = 1.0f / (2.0f * 3.141592653589793f * m_cur_warmth);
        const float alpha_lpf = dt / (rc + dt);

        // --- ЛЕВЫЙ КАНАЛ ---
        float xl = in_l[i];
        float in_sat_l = std::tanh(xl * m_cur_drive);
        float v_plate_l = std::tanh(in_sat_l + m_cur_bias) - tanh_bias;
        float norm_l = v_plate_l * inv_neg_swing;

        // Цифровой межкаскадный конденсатор (15 Гц DC-Blocker)
        float safe_tube_l = norm_l - m_dc_x_l + r_dc * m_dc_y_l;
        m_dc_x_l = norm_l;
        m_dc_y_l = safe_tube_l;

        // Суммирование Dry/Wet без гребенчатой фильтрации
        float dry_wet_l = xl * (1.0f - m_cur_mix) + safe_tube_l * m_cur_mix;
        // Мягкий выходной ФНЧ поверх всего сигнала
        m_post_lpf_l += alpha_lpf * (dry_wet_l - m_post_lpf_l);
        out_l[i] = m_post_lpf_l;

        // --- ПРАВЫЙ КАНАЛ ---
        float xr = in_r[i];
        float in_sat_r = std::tanh(xr * m_cur_drive);
        float v_plate_r = std::tanh(in_sat_r + m_cur_bias) - tanh_bias;
        float norm_r = v_plate_r * inv_neg_swing;

        // Цифровой межкаскадный конденсатор (15 Гц DC-Blocker)
        float safe_tube_r = norm_r - m_dc_x_r + r_dc * m_dc_y_r;
        m_dc_x_r = norm_r;
        m_dc_y_r = safe_tube_r;

        // Суммирование Dry/Wet без гребенчатой фильтрации
        float dry_wet_r = xr * (1.0f - m_cur_mix) + safe_tube_r * m_cur_mix;
        // Мягкий выходной ФНЧ поверх всего сигнала
        m_post_lpf_r += alpha_lpf * (dry_wet_r - m_post_lpf_r);
        out_r[i] = m_post_lpf_r;
    }
}
