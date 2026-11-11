#include "faust_dsp.hpp"
#include <cmath>
#include <algorithm>

static constexpr float PHI_Q = 0.6180339887f; // 1 / phi

FaustDSP::FaustDSP() {}

void FaustDSP::init(uint32_t sample_rate) {
    m_sample_rate = sample_rate ? sample_rate : 48000;
    m_dc_x_l = m_dc_y_l = 0.0f;
    m_dc_x_r = m_dc_y_r = 0.0f;

    m_cur_drive = m_target_drive.load(std::memory_order_relaxed);
    m_cur_bias = m_target_bias.load(std::memory_order_relaxed);
    m_cur_mix = m_target_mix.load(std::memory_order_relaxed);

    const float fs = static_cast<float>(m_sample_rate);
    m_eq_sub.updatePeak(55.0f, 0.0f, PHI_Q, fs);
    m_eq_warmth.updatePeak(377.0f, 0.0f, PHI_Q, fs);
    m_eq_presence.updatePeak(2584.0f, 0.0f, PHI_Q, fs);
    m_eq_air.updatePeak(6765.0f, 0.0f, PHI_Q, fs);
}

void FaustDSP::process(const float* in_l, const float* in_r, float* out_l, float* out_r, size_t frames) {
    if (m_bypass.load(std::memory_order_relaxed)) {
        std::copy(in_l, in_l + frames, out_l);
        std::copy(in_r, in_r + frames, out_r);
        return;
    }

    const float fs = static_cast<float>(m_sample_rate);
    const float block_smoo = 1.0f - std::exp(-static_cast<float>(frames) / (0.025f * fs));

    // k-rate: Сглаживание сатуратора
    m_cur_drive += block_smoo * (m_target_drive.load(std::memory_order_relaxed) - m_cur_drive);
    m_cur_bias  += block_smoo * (m_target_bias.load(std::memory_order_relaxed)  - m_cur_bias);
    m_cur_mix   += block_smoo * (m_target_mix.load(std::memory_order_relaxed)   - m_cur_mix);

    const float cur_drive = m_cur_drive;
    const float cur_bias  = m_cur_bias;
    const float cur_mix   = m_cur_mix;
    const float cur_dry   = 1.0f - cur_mix;

    // k-rate: Расчет нормализации триода
    const float tanh_bias = fast_tanh(cur_bias);
    const float max_neg = std::abs(fast_tanh(-1.0f * cur_drive + cur_bias) - tanh_bias) + 1e-6f;
    const float inv_neg = 1.0f / max_neg;
    const float r_dc = 1.0f - (2.0f * 3.141592653589793f * 15.0f / fs);

    // k-rate: Обновление фильтров Phi-EQ при изменении уставки
    const float tg_sub = m_target_g_sub.load(std::memory_order_relaxed);
    if (std::abs(tg_sub - m_cur_g_sub) > 0.02f) {
        m_cur_g_sub += block_smoo * (tg_sub - m_cur_g_sub);
        m_eq_sub.updatePeak(55.0f, m_cur_g_sub, PHI_Q, fs);
    }
    const float tg_warmth = m_target_g_warmth.load(std::memory_order_relaxed);
    if (std::abs(tg_warmth - m_cur_g_warmth) > 0.02f) {
        m_cur_g_warmth += block_smoo * (tg_warmth - m_cur_g_warmth);
        m_eq_warmth.updatePeak(377.0f, m_cur_g_warmth, PHI_Q, fs);
    }
    const float tg_presence = m_target_g_presence.load(std::memory_order_relaxed);
    if (std::abs(tg_presence - m_cur_g_presence) > 0.02f) {
        m_cur_g_presence += block_smoo * (tg_presence - m_cur_g_presence);
        m_eq_presence.updatePeak(2584.0f, m_cur_g_presence, PHI_Q, fs);
    }
    const float tg_air = m_target_g_air.load(std::memory_order_relaxed);
    if (std::abs(tg_air - m_cur_g_air) > 0.02f) {
        m_cur_g_air += block_smoo * (tg_air - m_cur_g_air);
        m_eq_air.updatePeak(6765.0f, m_cur_g_air, PHI_Q, fs);
    }

    // a-rate: Быстрый стерео-аудиоцикл
    for (size_t i = 0; i < frames; ++i) {
        // --- ЛЕВЫЙ КАНАЛ ---
        const float xl = in_l[i];
        const float v_plate_l = fast_tanh(fast_tanh(xl * cur_drive) + cur_bias) - tanh_bias;
        const float norm_l = v_plate_l * inv_neg;
        const float safe_l = norm_l - m_dc_x_l + r_dc * m_dc_y_l;
        m_dc_x_l = norm_l;
        m_dc_y_l = safe_l;
        float sig_l = xl * cur_dry + safe_l * cur_mix;

        // --- ПРАВЫЙ КАНАЛ ---
        const float xr = in_r[i];
        const float v_plate_r = fast_tanh(fast_tanh(xr * cur_drive) + cur_bias) - tanh_bias;
        const float norm_r = v_plate_r * inv_neg;
        const float safe_r = norm_r - m_dc_x_r + r_dc * m_dc_y_r;
        m_dc_x_r = norm_r;
        m_dc_y_r = safe_r;
        float sig_r = xr * cur_dry + safe_r * cur_mix;

        // Прогон через каскад 4 биквадратов Phi-EQ
        m_eq_sub.process(sig_l, sig_r, sig_l, sig_r);
        m_eq_warmth.process(sig_l, sig_r, sig_l, sig_r);
        m_eq_presence.process(sig_l, sig_r, sig_l, sig_r);
        m_eq_air.process(sig_l, sig_r, out_l[i], out_r[i]);
    }
}
