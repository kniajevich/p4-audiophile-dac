declare name "AudiophileTubeP4";
declare author "Alex (kniajevich)";
declare license "GNU AGPLv3";

import("stdfaust.lib");

// Сглаженные параметры управления (убирает щелчки при регулировке)
drive   = hslider("Drive", 1.2, 1.0, 4.0, 0.01) : si.smoo;
bias    = hslider("Grid_Bias", 0.18, 0.0, 0.4, 0.005) : si.smoo;
warmth  = hslider("Warmth_Hz", 16000, 4000, 20000, 10) : si.smoo;
mix     = hslider("Dry_Wet", 0.5, 0.0, 1.0, 0.01) : si.smoo;

// Асимметричный ламповый триод с безопасной нормализацией
triode(x) = safe_out
with {
    // Входной сигнал с легким софт-клипом
    in_sat = ma.tanh(x * drive);
    
    // Смещение рабочей точки сетки триода
    v_grid = in_sat + bias;
    
    // Асимметричная передаточная функция (эмуляция ВАХ триода)
    v_plate = ma.tanh(v_grid) - ma.tanh(bias);
    
    // Автобалансировка коэффициента усиления: расчет максимального отрицательного размаха
    max_neg_swing = abs(ma.tanh(-1.0 * drive + bias) - ma.tanh(bias)) + 1e-6;
    
    // Нормализация строго в диапазон [-1.0, +1.0]
    norm_signal = v_plate / max_neg_swing;
    
    // Цифровой межкаскадный конденсатор (отсекаем динамическую постоянку ниже 15 Гц)
    safe_out = norm_signal : fi.dcblockerat(15.0);
};

// Обработка одного канала без фазового расслоения
tube_channel(x) = (dry_wet : post_filter)
with {
    // Чистый микс без гребенчатой дефазировки
    dry_wet = x * (1.0 - mix) + triode(x) * mix;
    
    // Мягкий Low-Pass на выходе всего каскада (без расфазировки с dry)
    post_filter = fi.lowpass(1, warmth);
};

// Стерео-процессинг
process = tube_channel, tube_channel;
