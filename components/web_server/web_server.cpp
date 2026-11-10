#include "web_server.hpp"
#include <algorithm>
#include <vector>
#include <cstring>
#include "esp_log.h"
#include "dsp_manager.hpp"
#include "network_manager.hpp"
#include "ota_manager.hpp"
#include "sd_storage.hpp"
#include "esp_system.h"

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
    .brand h1 {
      margin: 0; font-family: 'Cinzel Decorative', Georgia, serif; font-size: 18px; font-weight: 700;
      letter-spacing: 0.14em; text-transform: uppercase;
      background: var(--gold-gradient); -webkit-background-clip: text; -webkit-text-fill-color: transparent;
    }
    .brand .script-sub {
      font-family: 'Alex Brush', cursive; font-size: 16px; color: var(--gold-bright); opacity: 0.85;
    }
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
    .tube-glass-bay {
      position: relative; background: rgba(12, 14, 18, 0.75); backdrop-filter: blur(14px);
      border: 1px solid rgba(212, 175, 55, 0.22); border-radius: 8px; padding: 18px 14px;
      overflow: hidden;
    }
    .tube-glow-ambient {
      position: absolute; top: -50%; left: -20%; width: 140%; height: 200%;
      background: radial-gradient(circle at 50% 50%, rgba(220, 38, 38, 0.22) 0%, transparent 70%);
      animation: tubeBreathing 3.236s infinite ease-in-out; pointer-events: none;
    }
    @keyframes tubeBreathing {
      0%, 100% { opacity: 0.35; transform: scale(0.96); }
      50% { opacity: 0.85; transform: scale(1.05); }
    }
    .tube-glass-bay > * { position: relative; z-index: 1; }
    .knobs-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; margin-top: 4px; }
    @media (max-width: 600px) { .knobs-grid { grid-template-columns: repeat(2, 1fr); gap: 18px; } }
    .knob-pod { display: flex; flex-direction: column; align-items: center; gap: 8px; cursor: pointer; }
    .knob-bezel {
      width: 80px; height: 80px; border-radius: 50%; background: #0e1014; border: 1.5px solid var(--gold);
      display: flex; justify-content: center; align-items: center;
    }
    .knob-rotor {
      width: calc(100% - 6px); height: calc(100% - 6px); border-radius: 50%;
      background: radial-gradient(circle at 45% 45%, #1f222b 0%, #0c0d12 80%);
      position: relative; transform: rotate(0deg);
    }
    .knob-dot {
      width: 7px; height: 7px; border-radius: 50%; background: var(--ruby);
      position: absolute; top: 6px; left: calc(50% - 3.5px);
    }
    .knob-label { font-size: 12px; font-weight: 700; color: var(--text-dim); text-transform: uppercase; }
    .knob-value { font-size: 13px; font-family: monospace; font-weight: 700; color: var(--gold-bright); }
    .input-field {
      background: #08090c; border: 1px solid #1a1e28; border-radius: 4px; color: var(--text-main);
      padding: 8px 12px; font-size: 13px; width: 100%;
    }
    .gold-btn {
      background: var(--gold-gradient); color: #121316; font-weight: 700; font-size: 11px; border: none;
      padding: 9px 18px; border-radius: 4px; cursor: pointer; text-transform: uppercase; letter-spacing: 0.08em;
    }
    .grid-2col { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; }
    @media (max-width: 600px) { .grid-2col { grid-template-columns: 1fr; } }
  </style>
</head>
<body>
  <div class="chassis">
    <div class="faceplate-header">
      <div class="brand">
        <h1>JC-ESP32P4 &bull; Reference DAC</h1>
        <div class="script-sub">Audiophile Sound Reproduction Engine</div>
      </div>
      <div id="netBadge" style="font-size: 11px; color: var(--gold);">SYNCED</div>
    </div>

    <div class="tube-glass-bay">
      <div class="tube-glow-ambient"></div>
      <div class="knobs-grid">
        <div class="knob-pod" id="podVol"><div class="knob-bezel"><div class="knob-rotor" id="rotorVol"><div class="knob-dot"></div></div></div><div class="knob-label">Master</div><div class="knob-value" id="valVol">85%</div></div>
        <div class="knob-pod" id="podExciter"><div class="knob-bezel"><div class="knob-rotor" id="rotorExciter"><div class="knob-dot"></div></div></div><div class="knob-label">&Phi; Warmth</div><div class="knob-value" id="valExciter">45%</div></div>
        <div class="knob-pod" id="podBass"><div class="knob-bezel"><div class="knob-rotor" id="rotorBass"><div class="knob-dot"></div></div></div><div class="knob-label">ViPER Bass</div><div class="knob-value" id="valBass">60%</div></div>
        <div class="knob-pod" id="podConv"><div class="knob-bezel"><div class="knob-rotor" id="rotorConv"><div class="knob-dot"></div></div></div><div class="knob-label">IRS Convolver</div><div class="knob-value" id="valConv">35%</div></div>
      </div>
    </div>

    <!-- Dual Network Manager -->
    <div class="grid-2col">
      <div class="rack-card">
        <h2><span>Direct SoftAP Hotspot</span><span style="font-size:10px; color:var(--ruby);">WPA3-SAE</span></h2>
        <div style="display:flex; flex-direction:column; gap:8px;">
          <input type="text" id="apSsid" class="input-field" placeholder="Hotspot SSID (e.g. JC-P4-DAC)" />
          <input type="password" id="apPass" class="input-field" placeholder="WPA3 Pass (min 8 chars)" />
          <button class="gold-btn" onclick="saveHotspot()">Save Hotspot</button>
        </div>
        <div id="apStatus" style="font-size:11px; font-family:monospace; color:var(--text-dim);"></div>
      </div>

      <div class="rack-card">
        <h2><span>Client Wi-Fi Link</span><span style="font-size:10px; color:var(--gold);">STATION</span></h2>
        <div style="display:flex; flex-direction:column; gap:8px;">
          <input type="text" id="wifiSsid" class="input-field" placeholder="Home Router SSID" />
          <input type="password" id="wifiPass" class="input-field" placeholder="Router Password" />
          <button class="gold-btn" onclick="connectRouter()">Connect to Router</button>
        </div>
        <div id="wifiStatus" style="font-size:11px; font-family:monospace; color:var(--text-dim);"></div>
      </div>
    </div>
  </div>

  <script>
    let dsp = { vol: 0.85, exciter: 0.45, bass: 0.60, conv: 0.35 };
    function setupKnob(p, r, v, k) {
      let sy = 0, sv = 0;
      const rotor = document.getElementById(r), val = document.getElementById(v);
      const update = (n) => { rotor.style.transform = `rotate(${-135 + n*270}deg)`; val.innerText = `${Math.round(n*100)}%`; };
      update(dsp[k]);
      document.getElementById(p).addEventListener('mousedown', (e) => {
        sy = e.clientY; sv = dsp[k];
        const move = (ev) => { dsp[k] = Math.max(0, Math.min(1, sv + (sy - ev.clientY)/160)); update(dsp[k]); };
        const up = () => { window.removeEventListener('mousemove', move); window.removeEventListener('mouseup', up); };
        window.addEventListener('mousemove', move); window.addEventListener('mouseup', up);
      });
    }
    setupKnob('podVol', 'rotorVol', 'valVol', 'vol');
    setupKnob('podExciter', 'rotorExciter', 'valExciter', 'exciter');
    setupKnob('podBass', 'rotorBass', 'valBass', 'bass');
    setupKnob('podConv', 'rotorConv', 'valConv', 'conv');

    function saveHotspot() {
      const ssid = document.getElementById('apSsid').value.trim();
      const pass = document.getElementById('apPass').value.trim();
      if (!ssid || pass.length < 8) return alert('SSID is required; WPA3 password must be at least 8 characters.');
      const stat = document.getElementById('apStatus');
      stat.innerText = 'Applying Hotspot credentials...';
      fetch('/api/softap', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`
      }).then(r => r.text()).then(() => {
        stat.innerText = 'Hotspot updated. Reconnect Wi-Fi using new password.';
      }).catch(() => { stat.innerText = 'Error applying Hotspot settings'; });
    }

    function connectRouter() {
      const ssid = document.getElementById('wifiSsid').value.trim();
      const pass = document.getElementById('wifiPass').value.trim();
      if (!ssid) return alert('Enter router SSID');
      const stat = document.getElementById('wifiStatus');
      stat.innerText = 'Connecting to ' + ssid + '...';
      fetch('/api/wifi', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`
      }).then(r => r.text()).then(() => {
        stat.innerText = 'Saved. Switch smartphone to home router Wi-Fi.';
      }).catch(() => { stat.innerText = 'Reconnecting...'; });
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

static esp_err_t softap_post_handler(httpd_req_t *req) {
    apply_stealth_headers(req);
    char buf[256] = {0};
    int ret = httpd_req_recv(req, buf, std::min(req->content_len, (size_t)(sizeof(buf) - 1)));
    if (ret <= 0) return ESP_FAIL;

    std::string data(buf);
    std::string ssid, pass;
    size_t s_pos = data.find("ssid=");
    size_t p_pos = data.find("&pass=");
    if (s_pos != std::string::npos && p_pos != std::string::npos) {
        ssid = data.substr(s_pos + 5, p_pos - (s_pos + 5));
        pass = data.substr(p_pos + 6);
    }

    if (!ssid.empty() && pass.length() >= 8) {
        ESP_LOGI(TAG, "SoftAP updated: SSID=%s", ssid.c_str());
        httpd_resp_sendstr(req, "OK");
        vTaskDelay(pdMS_TO_TICKS(400));
        NetworkManager::instance().setAPCredentials(ssid, pass);
        return ESP_OK;
    }
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Password must be >= 8 chars");
    return ESP_FAIL;
}

static esp_err_t wifi_post_handler(httpd_req_t *req) {
    apply_stealth_headers(req);
    char buf[256] = {0};
    int ret = httpd_req_recv(req, buf, std::min(req->content_len, (size_t)(sizeof(buf) - 1)));
    if (ret <= 0) return ESP_FAIL;

    std::string data(buf);
    std::string ssid, pass;
    size_t s_pos = data.find("ssid=");
    size_t p_pos = data.find("&pass=");
    if (s_pos != std::string::npos && p_pos != std::string::npos) {
        ssid = data.substr(s_pos + 5, p_pos - (s_pos + 5));
        pass = data.substr(p_pos + 6);
    }

    if (!ssid.empty()) {
        ESP_LOGI(TAG, "Station target updated: SSID=%s", ssid.c_str());
        httpd_resp_sendstr(req, "OK");
        vTaskDelay(pdMS_TO_TICKS(500));
        NetworkManager::instance().setCredentials(ssid, pass);
        return ESP_OK;
    }
    httpd_resp_send_500(req);
    return ESP_FAIL;
}

esp_err_t WebServer::start(uint16_t port) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.stack_size = 10240;

    if (httpd_start(&m_server, &config) != ESP_OK) return ESP_FAIL;

    httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler, .user_ctx = nullptr };
    httpd_register_uri_handler(m_server, &root_uri);

    httpd_uri_t softap_uri = { .uri = "/api/softap", .method = HTTP_POST, .handler = softap_post_handler, .user_ctx = nullptr };
    httpd_register_uri_handler(m_server, &softap_uri);

    httpd_uri_t wifi_uri = { .uri = "/api/wifi", .method = HTTP_POST, .handler = wifi_post_handler, .user_ctx = nullptr };
    httpd_register_uri_handler(m_server, &wifi_uri);

    return ESP_OK;
}

void WebServer::stop() {
    if (m_server) {
        httpd_stop(m_server);
        m_server = nullptr;
    }
}
