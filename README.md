# p4-audiophile-dac: Hi-Fi Network Streamer & ViPER DSP Platform for ESP32-P4

High-resolution audiophile streaming preamplifier and DSP engine built exclusively for the **dual-core RISC-V ESP32-P4 SoC** (400 MHz) and **TI PCM5102A 32-bit/384kHz DAC**.

---

## Hardware Wiring (ESP32-P4 to PCM5102A)

| PCM5102A Pin | ESP32-P4 GPIO | Description |
| :--- | :--- | :--- |
| **VCC** | **3.3V / 5.0V** | Board Power Supply |
| **GND** | **GND** | Ground Reference |
| **BCK** | **GPIO 12** | I2S Bit Clock (BCLK) |
| **DIN** | **GPIO 11** | I2S Serial Data Out (DOUT) |
| **LCK** | **GPIO 13** | I2S Word Select (WS / LRCK) |
| **SCK** | **GND** | Tied to GND to enable internal hardware PLL |

### Yellow Board Solder Jumpers (Bottom Side):
* **H1L (FLT):** Solder to `L` (Low-latency FIR filter)
* **H2L (DEMP):** Solder to `L` (De-emphasis Off)
* **H3L (FMT):** Solder to `L` (Standard I2S Philips mode)
* **H4L (XMT):** Solder to `H` (Soft Mute disabled / Active)
