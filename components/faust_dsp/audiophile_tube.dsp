declare name "AudiophileTubePhi";
declare author "Alex (kniajevich)";
declare license "GNU AGPLv3";

import("stdfaust.lib");

phi = (1.0 + sqrt(5.0)) / 2.0;
q_phi = 1.0 / phi;

// Fibonacci Frequencies (Hz)
f_sub      = 55.0;
f_warmth   = 377.0;
f_presence = 2584.0;
f_air      = 6765.0;

// Saturation parameters
drive   = hslider("Drive", 1.2, 1.0, 4.0, 0.01) : si.smoo;
bias    = hslider("Grid_Bias", 0.18, 0.0, 0.4, 0.005) : si.smoo;
mix     = hslider("Dry_Wet", 0.5, 0.0, 1.0, 0.01) : si.smoo;

// Phi-EQ Band Gains (dB)
g_sub      = hslider("Phi_Sub_dB", 0.0, -6.0, 6.0, 0.1) : si.smoo;
g_warmth   = hslider("Phi_Warmth_dB", 0.0, -6.0, 6.0, 0.1) : si.smoo;
g_presence = hslider("Phi_Presence_dB", 0.0, -6.0, 6.0, 0.1) : si.smoo;
g_air      = hslider("Phi_Air_dB", 0.0, -6.0, 6.0, 0.1) : si.smoo;

triode(x) = (v_plate / max_neg) : fi.dcblockerat(15.0)
with {
    v_plate = ma.tanh(ma.tanh(x * drive) + bias) - ma.tanh(bias);
    max_neg = abs(ma.tanh(-1.0 * drive + bias) - ma.tanh(bias)) + 1e-6;
};

eq_section = fi.peak_eq_cq(g_sub, f_sub, q_phi) :
             fi.peak_eq_cq(g_warmth, f_warmth, q_phi) :
             fi.peak_eq_cq(g_presence, f_presence, q_phi) :
             fi.peak_eq_cq(g_air, f_air, q_phi);

tube_chain(x) = (x * (1.0 - mix) + triode(x) * mix) : eq_section;
process = tube_chain, tube_chain;
