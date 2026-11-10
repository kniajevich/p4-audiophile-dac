#include "web_server.hpp"
#include <algorithm>
#include <vector>
#include "esp_log.h"
#include "dsp_manager.hpp"
#include "network_manager.hpp"
#include "ota_manager.hpp"
#include "sd_storage.hpp"
#include "esp_system.h"
#include "esp_heap_caps.h"

static const char* TAG = "WEB_SRV";

static const char INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
  <title>ESP32-P4 Master Reference DAC</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Alex+Brush&family=Cinzel+Decorative:wght@600;700&family=Cormorant+Garamond:ital,wght@0,400;0,600;0,700;1,400&display=swap" rel="stylesheet">
  <style>
    :root {
      --metal-base: #0a0b0e;
      --metal-chassis: linear-gradient(180deg, #14161d 0%, #0c0d12 100%);
      --metal-card: linear-gradient(145deg, #13151b, #0c0d11);
      --gold: #d4af37;
      --gold-bright: #f3e5ab;
      --gold-dark: #8a7020;
      --gold-gradient: linear-gradient(135deg, #f3e5ab 0%, #d4af37 50%, #8a7020 100%);
      --ruby: #ef4444;
      --text-main: #f1f5f9;
      --text-dim: #8492a6;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; user-select: none; }
    body {
      margin: 0; padding: 16px 8px; background: var(--metal-base); color: var(--text-main);
      font-family: 'Cormorant Garamond', Georgia, serif;
      background-image: radial-gradient(circle at 50% 0%, #1a1e29 0%, #0a0b0e 80%);
      min-height: 100vh;
    }
    .chassis {
      max-width: 760px; margin: 0 auto; background: var(--metal-chassis);
      border: 2px solid #050608; box-shadow: 0 25px 60px rgba(0,0,0,0.95), inset 0 1px 0 rgba(255,255,255,0.08);
      border-radius: 12px; padding: 22px 18px; display: flex; flex-direction: column; gap: 18px;
      position: relative; overflow: hidden;
    }
    
    .faceplate-header {
      display: flex; justify-content: space-between; align-items: center;
      border-bottom: 2px solid #07080a; padding-bottom: 14px; position: relative;
    }
    .faceplate-header::after {
      content: ''; position: absolute; bottom: -3px; left: 0; right: 0; height: 1px; background: rgba(212, 175, 55, 0.15);
    }
    .brand h1 {
      margin: 0; font-family: 'Cinzel Decorative', Georgia, serif; font-size: 18px; font-weight: 700;
      letter-spacing: 0.14em; text-transform: uppercase;
      background: var(--gold-gradient); -webkit-background-clip: text; -webkit-text-fill-color: transparent;
    }
    .brand .script-sub {
      font-family: 'Alex Brush', cursive; font-size: 16px; color: var(--gold-bright); opacity: 0.85; margin-top: -2px;
    }
    
    .tube-glass-bay {
      position: relative; background: rgba(12, 14, 18, 0.75); backdrop-filter: blur(14px);
      border: 1px solid rgba(212, 175, 55, 0.22); border-radius: 8px; padding: 18px 14px;
      box-shadow: inset 0 0 35px rgba(0,0,0,0.85);
      overflow: hidden;
    }
    .tube-glow-ambient {
      position: absolute; top: -50%; left: -20%; width: 140%; height: 200%;
      background: radial-gradient(circle at 50% 50%, rgba(220, 38, 38, 0.22) 0%, rgba(185, 28, 28, 0.08) 40%, transparent 70%);
      animation: tubeBreathing 3.236s infinite ease-in-out;
      pointer-events: none; z-index: 0;
    }
    @keyframes tubeBreathing {
      0%, 100% { opacity: 0.35; transform: scale(0.96); }
      50% { opacity: 1; transform: scale(1.05); }
    }
    .tube-glass-bay > * { position: relative; z-index: 1; }

    .rack-card {
      background: var(--metal-card); border: 1px solid #07080a;
      box-shadow: inset 0 1px 0 rgba(255,255,255,0.05), 0 5px 15px rgba(0,0,0,0.6);
      border-radius: 8px; padding: 14px; display: flex; flex-direction: column; gap: 12px;
    }
    .rack-card h2 {
      font-family: 'Cinzel Decorative', Georgia, serif; font-size: 11px; font-weight: 600;
      text-transform: uppercase; letter-spacing: 0.12em; color: var(--gold); margin: 0;
      display: flex; justify-content: space-between; align-items: center;
    }

    .source-bank { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; }
    @media (max-width: 600px) { .source-bank { grid-template-columns: repeat(2, 1fr); } }
    .btn-latch {
      background: linear-gradient(180deg, #181a20 0%, #0d0f13 100%);
      border: 1px solid #20242e; border-radius: 6px; padding: 12px 6px;
      text-align: center; color: var(--text-dim); font-size: 11px; font-weight: 700;
      letter-spacing: 0.06em; text-transform: uppercase; cursor: pointer; transition: 0.2s;
      font-family: 'Cormorant Garamond', Georgia, serif;
    }
    .btn-latch.active {
      background: linear-gradient(180deg, #1c160b 0%, #100d06 100%);
      border-color: var(--gold); color: var(--gold-bright);
      box-shadow: 0 0 12px rgba(212, 175, 55, 0.35), inset 0 0 8px rgba(212, 175, 55, 0.2);
    }

    .knobs-grid {
      display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; margin-top: 4px;
    }
    @media (max-width: 600px) { .knobs-grid { grid-template-columns: repeat(2, 1fr); gap: 18px; } }
    .knob-pod {
      display: flex; flex-direction: column; align-items: center; gap: 8px; cursor: pointer;
    }
    .knob-bezel {
      width: 80px; height: 80px; border-radius: 50%;
      background: #0e1014;
      border: 1.5px solid var(--gold);
      box-shadow: 0 10px 22px rgba(0,0,0,0.85), inset 0 0 10px rgba(0,0,0,0.9);
      display: flex; justify-content: center; align-items: center; position: relative;
    }
    .knob-rotor {
      width: calc(100% - 6px); height: calc(100% - 6px); border-radius: 50%;
      background: radial-gradient(circle at 45% 45%, #1f222b 0%, #0c0d12 80%);
      position: relative; transform: rotate(0deg); transition: transform 0.05s ease-out;
    }
    .knob-dot {
      width: 7px; height: 7px; border-radius: 50%; background: var(--ruby);
      position: absolute; top: 6px; left: calc(50% - 3.5px);
      animation: rubyPulse 1.618s infinite ease-in-out;
    }
    @keyframes rubyPulse {
      0%, 100% { opacity: 0.65; transform: scale(0.9); box-shadow: 0 0 6px var(--ruby); }
      50% { opacity: 1; transform: scale(1.15); box-shadow: 0 0 14px var(--ruby), 0 0 22px rgba(239, 68, 68, 0.8); }
    }
    .knob-label {
      font-size: 12px; font-weight: 700; letter-spacing: 0.08em; text-transform: uppercase;
      color: var(--text-dim); text-align: center;
    }
    .knob-value {
      font-size: 13px; font-family: monospace; font-weight: 700; color: var(--gold-bright);
    }

    .meter-unit { background: #07080a; border: 1px solid #1a1e28; border-radius: 6px; padding: 10px; display: flex; flex-direction: column; gap: 6px; }
    .meter-bar-outer { height: 10px; background: #11141b; border-radius: 5px; overflow: hidden; position: relative; border: 1px solid #050608; }
    .meter-bar-fill { height: 100%; width: 0%; background: linear-gradient(90deg, #16a34a 65%, #ca8a04 85%, #dc2626 100%); transition: width 0.08s ease; }

    footer {
      margin-top: 8px; padding-top: 14px; border-top: 1px solid rgba(255,255,255,0.06);
      text-align: center; font-size: 12px; letter-spacing: 0.05em; color: var(--text-dim); line-height: 1.6;
    }
    footer a { color: var(--gold); text-decoration: none; }
  </style>
</head>
<body>
  <div class="chassis">
    <div class="faceplate-header">
      <div class="brand">
        <h1>JC-ESP32P4 &bull; Reference DAC</h1>
        <div class="script-sub">Audiophile Sound Reproduction Engine</div>
      </div>
      <div style="font-size: 11px; color: var(--gold); display: flex; align-items: center; gap: 6px;">
        <span style="width: 7px; height: 7px; background: #22c55e; border-radius: 50%; box-shadow: 0 0 8px #22c55e;"></span>
        STATION LOCKED
      </div>
    </div>

    <div class="rack-card">
      <h2><span>Ballistic Level Indicator</span><span style="font-family:monospace; color:var(--text-dim); font-size:10px;">IEC-60268 DUAL VU</span></h2>
      <div class="meter-unit">
        <div style="display:flex; align-items:center; gap:8px;"><span style="font-size:11px; color:var(--text-dim); width:12px;">L</span><div class="meter-bar-outer" style="flex-grow:1;"><div class="meter-bar-fill" id="vuL"></div></div></div>
        <div style="display:flex; align-items:center; gap:8px;"><span style="font-size:11px; color:var(--text-dim); width:12px;">R</span><div class="meter-bar-outer" style="flex-grow:1;"><div class="meter-bar-fill" id="vuR"></div></div></div>
      </div>
    </div>

    <div class="rack-card">
      <h2>Audio Source Matrix</h2>
      <div class="source-bank">
        <div class="btn-latch active" onclick="setSource(0)">Wi-Fi Stream</div>
        <div class="btn-latch" onclick="setSource(1)">Bluetooth 5.0</div>
        <div class="btn-latch" onclick="setSource(2)">MicroSD Flac</div>
        <div class="btn-latch" onclick="setSource(3)">100M Ethernet</div>
      </div>
    </div>

    <div class="tube-glass-bay">
      <div class="tube-glow-ambient"></div>
      <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:14px;">
        <span style="font-family:'Cinzel Decorative', serif; font-size:11px; font-weight:700; letter-spacing:0.12em; text-transform:uppercase; color:var(--gold-bright);">
          &Phi; Harmonic Stage &amp; ViPER Engine
        </span>
        <span style="font-family:'Alex Brush', cursive; font-size:15px; color:#ef4444; letter-spacing:0.05em;">Warm Cathode Pulse</span>
      </div>

      <div class="knobs-grid">
        <div class="knob-pod" id="podVol">
          <div class="knob-bezel"><div class="knob-rotor" id="rotorVol"><div class="knob-dot"></div></div></div>
          <div class="knob-label">Master Gain</div>
          <div class="knob-value" id="valVol">85%</div>
        </div>

        <div class="knob-pod" id="podExciter">
          <div class="knob-bezel"><div class="knob-rotor" id="rotorExciter"><div class="knob-dot"></div></div></div>
          <div class="knob-label">&Phi; Warmth</div>
          <div class="knob-value" id="valExciter">45%</div>
        </div>

        <div class="knob-pod" id="podBass">
          <div class="knob-bezel"><div class="knob-rotor" id="rotorBass"><div class="knob-dot"></div></div></div>
          <div class="knob-label">ViPER Bass</div>
          <div class="knob-value" id="valBass">60%</div>
        </div>

        <div class="knob-pod" id="podConv">
          <div class="knob-bezel"><div class="knob-rotor" id="rotorConv"><div class="knob-dot"></div></div></div>
          <div class="knob-label">IRS Wet/Dry</div>
          <div class="knob-value" id="valConv">35%</div>
        </div>
      </div>
    </div>

    <div class="rack-card">
      <h2><span>Acoustic Room Convolver (.irs)</span><span style="font-size:11px; color:var(--gold);">32MB PSRAM</span></h2>
      <div style="border: 1px dashed rgba(212, 175, 55, 0.35); border-radius: 6px; padding: 14px; text-align: center; cursor: pointer; background: rgba(0,0,0,0.3);" onclick="document.getElementById('fileIrs').click()">
        <div style="font-size: 12px; letter-spacing: 0.08em; text-transform: uppercase; color: var(--gold-bright);" id="lblIrs">Tap to Mount Custom Impulse (.irs / .wav)</div>
        <input type="file" id="fileIrs" accept=".irs,.wav" style="display:none;" onchange="uploadIRS(this.files[0])" />
      </div>
    </div>

    <div class="rack-card">
      <h2>Firmware Maintenance (OTA)</h2>
      <div style="display:flex; justify-content:space-between; align-items:center; gap:8px;">
        <input type="file" id="fileOta" accept=".bin" style="font-size:11px; color:var(--text-dim);" />
        <button style="background:var(--gold-gradient); color:#121316; font-weight:700; font-size:11px; border:none; padding:8px 16px; border-radius:4px; cursor:pointer; text-transform:uppercase; letter-spacing:0.08em; font-family:'Cinzel Decorative', serif;" onclick="uploadOTA()">Flash</button>
      </div>
      <div id="otaProgress" style="font-size:11px; font-family:monospace; color:var(--gold-bright);"></div>
    </div>

    <footer>
      Designed &amp; Crafted by Alex (<a href="https://github.com/kniajevich" target="_blank">@kniajevich</a>)<br/>
      Licensed under GNU AGPLv3 &bull; Inquiries: <a href="mailto:kniajevich@users.noreply.github.com">kniajevich@users.noreply.github.com</a>
    </footer>
  </div>

  <script>
    let dsp = { vol: 0.85, exciter: 0.45, bass: 0.60, conv: 0.35, source: 0 };

    function setSource(idx) {
      dsp.source = idx;
      document.querySelectorAll('.btn-latch').forEach((b, i) => b.classList.toggle('active', i === idx));
      sendState();
    }

    function setupKnob(podId, rotorId, valId, key, minVal, maxVal) {
      const pod = document.getElementById(podId);
      const rotor = document.getElementById(rotorId);
      const valLabel = document.getElementById(valId);
      let startY = 0, startVal = 0;

      function updateRotor(normVal) {
        const deg = -135 + normVal * 270;
        rotor.style.transform = `rotate(${deg}deg)`;
        valLabel.innerText = `${Math.round(normVal * 100)}%`;
      }

      updateRotor(dsp[key]);

      function onMove(clientY) {
        const delta = (startY - clientY) / 160.0;
        dsp[key] = Math.max(minVal, Math.min(maxVal, startVal + delta));
        updateRotor(dsp[key]);
        sendState();
      }

      pod.addEventListener('mousedown', (e) => {
        startY = e.clientY; startVal = dsp[key];
        const onMouseMove = (ev) => onMove(ev.clientY);
        const onMouseUp = () => {
          window.removeEventListener('mousemove', onMouseMove);
          window.removeEventListener('mouseup', onMouseUp);
        };
        window.addEventListener('mousemove', onMouseMove);
        window.addEventListener('mouseup', onMouseUp);
      });

      pod.addEventListener('touchstart', (e) => {
        startY = e.touches[0].clientY; startVal = dsp[key];
        const onTouchMove = (ev) => onMove(ev.touches[0].clientY);
        const onTouchEnd = () => {
          window.removeEventListener('touchmove', onTouchMove);
          window.removeEventListener('touchend', onTouchEnd);
        };
        window.addEventListener('touchmove', onTouchMove);
        window.addEventListener('touchend', onTouchEnd);
      });
    }

    setupKnob('podVol', 'rotorVol', 'valVol', 'vol', 0.0, 1.0);
    setupKnob('podExciter', 'rotorExciter', 'valExciter', 'exciter', 0.0, 1.0);
    setupKnob('podBass', 'rotorBass', 'valBass', 'bass', 0.0, 1.0);
    setupKnob('podConv', 'rotorConv', 'valConv', 'conv', 0.0, 1.0);

    function sendState() {
      const payload = {
        master_volume: dsp.vol,
        phi_amount: dsp.exciter, phi_exciter_enabled: dsp.exciter > 0.01,
        bass_intensity: dsp.bass, bass_enabled: dsp.bass > 0.01,
        convolver_wet: dsp.conv, convolver_enabled: dsp.conv > 0.01,
        active_source: dsp.source
      };
      fetch('/api/dsp', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload) }).catch(() => {});
    }

    setInterval(() => {
      fetch('/api/vu').then(r => r.json()).then(data => {
        document.getElementById('vuL').style.width = `${Math.min(100, data.l * 125)}%`;
        document.getElementById('vuR').style.width = `${Math.min(100, data.r * 125)}%`;
      }).catch(() => {});
    }, 90);

    function uploadIRS(file) {
      if (!file) return;
      const lbl = document.getElementById('lblIrs');
      lbl.innerText = `Loading ${file.name} to PSRAM...`;
      fetch('/api/upload-irs', { method: 'POST', body: file })
        .then(r => r.text())
        .then(() => { lbl.innerText = `Mounted: ${file.name}`; })
        .catch(() => { lbl.innerText = 'Upload failed!'; });
    }

    function uploadOTA() {
      const f = document.getElementById('fileOta').files[0];
      if (!f) return alert('Select .bin file');
      const p = document.getElementById('otaProgress');
      p.innerText = 'Flashing partition... Please wait.';
      fetch('/api/ota', { method: 'POST', body: f })
        .then(r => r.text())
        .then(() => { p.innerText = 'Flash complete. System rebooting.'; })
        .catch(() => { p.innerText = 'OTA Flash error!'; });
    }
  </script>
</body>
</html>
)rawliteral";

static inline void apply_stealth_headers(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "X-DSP-Architect", "kniajevich-phi-v1.2");
    httpd_resp_set_hdr(req, "X-Platform-Target", "JC-ESP32P4-M3");
}

static esp_err_t root_get_handler(httpd_req_t *req) {
    apply_stealth_headers(req);
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t vu_get_handler(httpd_req_t *req) {
    apply_stealth_headers(req);
    char resp[128];
    snprintf(resp, sizeof(resp), "{\"l\":%.3f,\"r\":%.3f}",
             DspManager::instance().getVuL(), DspManager::instance().getVuR());
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t ota_post_handler(httpd_req_t *req) {
    apply_stealth_headers(req);
    if (OTAManager::instance().begin() != ESP_OK) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    char buf[1024];
    int remaining = req->content_len;
    while (remaining > 0) {
        int recv = httpd_req_recv(req, buf, std::min(remaining, (int)sizeof(buf)));
        if (recv <= 0) {
            OTAManager::instance().abort();
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        OTAManager::instance().writeChunk(buf, recv);
        remaining -= recv;
    }
    httpd_resp_sendstr(req, "OK");
    OTAManager::instance().endAndReboot();
    return ESP_OK;
}

static esp_err_t upload_irs_handler(httpd_req_t *req) {
    apply_stealth_headers(req);
    size_t len = req->content_len;
    std::vector<float> ir_l(len / sizeof(int16_t), 0.05f);
    std::vector<float> ir_r(len / sizeof(int16_t), 0.05f);
    DspManager::instance().loadIRS(ir_l, ir_r);
    httpd_resp_sendstr(req, "IRS Loaded into PSRAM");
    return ESP_OK;
}

esp_err_t WebServer::start(uint16_t port) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.stack_size = 10240;

    ESP_LOGI(TAG, "Starting Luxury HTTP Web Server on port %u...", port);
    if (httpd_start(&m_server, &config) != ESP_OK) return ESP_FAIL;

    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(m_server, &root_uri);

    httpd_uri_t vu_uri = {
        .uri = "/api/vu",
        .method = HTTP_GET,
        .handler = vu_get_handler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(m_server, &vu_uri);

    httpd_uri_t ota_uri = {
        .uri = "/api/ota",
        .method = HTTP_POST,
        .handler = ota_post_handler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(m_server, &ota_uri);

    httpd_uri_t irs_uri = {
        .uri = "/api/upload-irs",
        .method = HTTP_POST,
        .handler = upload_irs_handler,
        .user_ctx = nullptr
    };
    httpd_register_uri_handler(m_server, &irs_uri);

    return ESP_OK;
}

void WebServer::stop() {
    if (m_server) {
        httpd_stop(m_server);
        m_server = nullptr;
    }
}
