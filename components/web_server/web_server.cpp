#include "web_server.hpp"
#include "esp_log.h"
#include "dsp_manager.hpp"
#include "network_manager.hpp"
#include "esp_system.h"
#include "esp_heap_caps.h"

static const char* TAG = "WEB_SRV";

static const char INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
  <title>ESP32-P4 Audiophile DAC Hub</title>
  <style>
    :root {
      --bg: #090d14; --card: #121824; --border: #1e293b;
      --accent: #38bdf8; --accent-glow: rgba(56, 189, 248, 0.25);
      --accent-green: #22c55e; --text: #f1f5f9; --text-muted: #94a3b8;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    body {
      margin: 0; padding: 12px; background: var(--bg); color: var(--text);
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    }
    .container { max-width: 720px; margin: 0 auto; display: flex; flex-direction: column; gap: 12px; }
    header { display: flex; justify-content: space-between; align-items: center; padding-bottom: 8px; border-bottom: 1px solid var(--border); }
    h1 { font-size: 16px; margin: 0; font-weight: 700; display: flex; align-items: center; gap: 8px; }
    .badge { font-size: 10px; padding: 2px 8px; border-radius: 12px; background: #0369a1; color: #fff; font-weight: 600; }
    
    .telemetry { display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px; }
    @media (max-width: 600px) { .telemetry { grid-template-columns: repeat(2, 1fr); } }
    .stat-card { background: var(--card); border: 1px solid var(--border); border-radius: 6px; padding: 8px; text-align: center; }
    .stat-val { font-size: 13px; font-weight: 700; color: var(--accent); font-family: monospace; }
    .stat-lbl { font-size: 9px; text-transform: uppercase; color: var(--text-muted); margin-top: 2px; }

    .card { background: var(--card); border: 1px solid var(--border); border-radius: 8px; padding: 12px; display: flex; flex-direction: column; gap: 10px; }
    .card h2 { font-size: 11px; text-transform: uppercase; letter-spacing: 0.05em; color: var(--text-muted); margin: 0; display: flex; justify-content: space-between; align-items: center; }

    .source-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px; }
    @media (max-width: 600px) { .source-grid { grid-template-columns: repeat(2, 1fr); } }
    .src-btn { background: #0b111a; border: 1px solid var(--border); border-radius: 6px; padding: 10px; text-align: center; color: var(--text-muted); font-size: 11px; font-weight: 600; cursor: pointer; }
    .src-btn.active { background: #1e293b; border-color: var(--accent); color: #fff; box-shadow: 0 0 8px var(--accent-glow); }

    .ctrl-row { display: flex; align-items: center; justify-content: space-between; font-size: 11px; font-family: monospace; }
    input[type=range] { flex-grow: 1; margin: 0 10px; accent-color: var(--accent); }
    .switch-wrap { display: flex; align-items: center; gap: 8px; font-size: 11px; font-weight: 600; }
    .toggle { width: 34px; height: 18px; background: #334155; border-radius: 10px; position: relative; cursor: pointer; }
    .toggle::after { content: ''; position: absolute; width: 14px; height: 14px; background: #fff; border-radius: 50%; top: 2px; left: 2px; transition: 0.2s; }
    .toggle.active { background: var(--accent-green); }
    .toggle.active::after { left: 18px; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>ESP32-P4 Hi-Fi DAC Hub <span class="badge">JC-ESP32P4-M3</span></h1>
      <div style="font-size: 11px; font-family: monospace; color: var(--accent-green);">● Online</div>
    </header>

    <div class="telemetry">
      <div class="stat-card"><div class="stat-val">400 MHz</div><div class="stat-lbl">RISC-V Dual</div></div>
      <div class="stat-card"><div class="stat-val">32 MB</div><div class="stat-lbl">PSRAM Available</div></div>
      <div class="stat-card"><div class="stat-val">32-bit / 384k</div><div class="stat-lbl">PCM5102A Stream</div></div>
      <div class="stat-card"><div class="stat-val">Wi-Fi 6 + ETH</div><div class="stat-lbl">Network Active</div></div>
    </div>

    <div class="card">
      <h2>Audio Source Selection</h2>
      <div class="source-grid">
        <div class="src-btn active" onclick="setSource(0)">Wi-Fi Stream (DLNA)</div>
        <div class="src-btn" onclick="setSource(1)">Bluetooth 5 (A2DP)</div>
        <div class="src-btn" onclick="setSource(2)">MicroSD (FLAC/DSD)</div>
        <div class="src-btn" onclick="setSource(3)">100M Ethernet</div>
      </div>
    </div>

    <div class="card">
      <h2>ViPER4Android DSP Engine (Hardware Accelerated)</h2>
      <div class="ctrl-row">
        <div class="switch-wrap"><span>Dynamic Bass</span><div class="toggle active" id="tgBass" onclick="toggleDSP('bass')"></div></div>
        <input type="range" id="slBass" min="0" max="100" value="60" oninput="updateDSP()" />
        <span id="valBass">60%</span>
      </div>
      <div class="ctrl-row">
        <div class="switch-wrap"><span>DDC 10-Band EQ</span><div class="toggle active" id="tgEq" onclick="toggleDSP('eq')"></div></div>
        <input type="range" id="slEq" min="-12" max="12" value="2" oninput="updateDSP()" />
        <span id="valEq">+2 dB</span>
      </div>
      <div class="ctrl-row">
        <span>Master Digital Gain</span>
        <input type="range" id="slVol" min="0" max="100" value="85" oninput="updateDSP()" />
        <span id="valVol">85%</span>
      </div>
    </div>

    <div class="card">
      <h2>Convolver Impulse Response (.irs Profile)</h2>
      <div class="ctrl-row">
        <span>IRS Profile:</span>
        <select id="selIrs" style="background:#0b111a; color:#fff; border:1px solid var(--border); padding:6px; border-radius:4px;">
          <option>Sony MDR-Z1R Acoustic Correction</option>
          <option>Studer A810 Master Tape Saturation</option>
          <option>Bowers & Wilkins 800 Diamond DDC</option>
          <option>Lexicon 480L Concert Hall 2.4s</option>
        </select>
      </div>
    </div>
  </div>

  <script>
    let dspState = { bass: true, bassVal: 0.6, eq: true, eqVal: 2.0, vol: 0.85, source: 0 };

    function setSource(idx) {
      dspState.source = idx;
      document.querySelectorAll('.src-btn').forEach((b, i) => b.classList.toggle('active', i === idx));
      sendState();
    }

    function toggleDSP(param) {
      dspState[param] = !dspState[param];
      document.getElementById(param === 'bass' ? 'tgBass' : 'tgEq').classList.toggle('active', dspState[param]);
      sendState();
    }

    function updateDSP() {
      dspState.bassVal = parseInt(document.getElementById('slBass').value) / 100.0;
      document.getElementById('valBass').innerText = `${Math.round(dspState.bassVal * 100)}%`;
      dspState.eqVal = parseFloat(document.getElementById('slEq').value);
      document.getElementById('valEq').innerText = `${dspState.eqVal > 0 ? '+' : ''}${dspState.eqVal} dB`;
      dspState.vol = parseInt(document.getElementById('slVol').value) / 100.0;
      document.getElementById('valVol').innerText = `${Math.round(dspState.vol * 100)}%`;
      sendState();
    }

    function sendState() {
      fetch('/api/dsp', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(dspState)
      }).catch(() => {});
    }
  </script>
</body>
</html>
)rawliteral";

static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t status_get_handler(httpd_req_t *req) {
    char resp[256];
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / (1024 * 1024);
    snprintf(resp, sizeof(resp), "{\"cpu_mhz\":400,\"psram_mb\":%u,\"ip\":\"%s\"}",
             (unsigned int)free_psram, NetworkManager::instance().getIpAddress().c_str());
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::start(uint16_t port) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.stack_size = 8192;

    ESP_LOGI(TAG, "Starting HTTP Web Server on port %u...", port);
    if (httpd_start(&m_server, &config) != ESP_OK) return ESP_FAIL;

    httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };
    httpd_register_uri_handler(m_server, &root_uri);

    httpd_uri_t status_uri = { .uri = "/api/status", .method = HTTP_GET, .handler = status_get_handler };
    httpd_register_uri_handler(m_server, &status_uri);

    return ESP_OK;
}

void WebServer::stop() {
    if (m_server) {
        httpd_stop(m_server);
        m_server = nullptr;
    }
}
