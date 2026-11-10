#include "web_server.hpp"
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
  <title>ESP32-P4 Audiophile DAC Hub Pro</title>
  <style>
    :root {
      --bg: #070a0f; --card: #0e1420; --border: #1a2538;
      --accent: #38bdf8; --accent-glow: rgba(56, 189, 248, 0.3);
      --accent-phi: #e3b341; --accent-green: #22c55e; --accent-red: #ef4444;
      --text: #f1f5f9; --text-muted: #94a3b8;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    body { margin: 0; padding: 12px; background: var(--bg); color: var(--text); font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; }
    .container { max-width: 740px; margin: 0 auto; display: flex; flex-direction: column; gap: 12px; }
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
    .src-btn { background: #070c14; border: 1px solid var(--border); border-radius: 6px; padding: 10px; text-align: center; color: var(--text-muted); font-size: 11px; font-weight: 600; cursor: pointer; }
    .src-btn.active { background: #162235; border-color: var(--accent); color: #fff; box-shadow: 0 0 8px var(--accent-glow); }

    .vu-box { display: flex; flex-direction: column; gap: 4px; padding: 8px; background: #070c14; border-radius: 6px; border: 1px solid var(--border); }
    .vu-line { display: flex; align-items: center; gap: 8px; font-size: 10px; font-family: monospace; }
    .vu-bar { flex-grow: 1; height: 8px; background: #1e293b; border-radius: 4px; overflow: hidden; position: relative; }
    .vu-fill { height: 100%; width: 0%; background: linear-gradient(90deg, #22c55e 70%, #e3b341 85%, #ef4444 100%); transition: width 0.08s ease; }

    .drop-zone { border: 2px dashed var(--border); border-radius: 8px; padding: 18px; text-align: center; font-size: 11px; color: var(--text-muted); cursor: pointer; transition: 0.2s; background: #070c14; }
    .drop-zone.hover { border-color: var(--accent); background: #0e1e30; }

    .ctrl-row { display: flex; align-items: center; justify-content: space-between; font-size: 11px; font-family: monospace; }
    input[type=range] { flex-grow: 1; margin: 0 10px; accent-color: var(--accent); }
    .switch-wrap { display: flex; align-items: center; gap: 8px; font-size: 11px; font-weight: 600; }
    .toggle { width: 34px; height: 18px; background: #263345; border-radius: 10px; position: relative; cursor: pointer; }
    .toggle::after { content: ''; position: absolute; width: 14px; height: 14px; background: #fff; border-radius: 50%; top: 2px; left: 2px; transition: 0.2s; }
    .toggle.active { background: var(--accent-green); }
    .toggle.active::after { left: 18px; }

    footer { text-align: center; font-size: 10px; color: var(--text-muted); border-top: 1px solid var(--border); padding-top: 10px; line-height: 1.5; }
    footer a { color: var(--accent); text-decoration: none; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>ESP32-P4 Audiophile DAC Hub <span class="badge">JC-ESP32P4-M3</span></h1>
      <div style="font-size: 11px; font-family: monospace; color: var(--accent-green);">● Online</div>
    </header>

    <div class="telemetry">
      <div class="stat-card"><div class="stat-val">400 MHz</div><div class="stat-lbl">Dual RISC-V</div></div>
      <div class="stat-card"><div class="stat-val">32 MB</div><div class="stat-lbl">PSRAM Buffer</div></div>
      <div class="stat-card"><div class="stat-val">32-bit / 384k</div><div class="stat-lbl">PCM5102A Direct</div></div>
      <div class="stat-card"><div class="stat-val">Wi-Fi 6 + ETH</div><div class="stat-lbl">Dual Net Link</div></div>
    </div>

    <div class="card">
      <h2>Live Hardware VU Meter</h2>
      <div class="vu-box">
        <div class="vu-line"><span>L</span><div class="vu-bar"><div class="vu-fill" id="vuL"></div></div></div>
        <div class="vu-line"><span>R</span><div class="vu-bar"><div class="vu-fill" id="vuR"></div></div></div>
      </div>
    </div>

    <div class="card">
      <h2>Audio Source Routing</h2>
      <div class="source-grid">
        <div class="src-btn active" onclick="setSource(0)">Wi-Fi Stream (DLNA)</div>
        <div class="src-btn" onclick="setSource(1)">Bluetooth 5 (A2DP)</div>
        <div class="src-btn" onclick="setSource(2)">MicroSD (FLAC/DSD)</div>
        <div class="src-btn" onclick="setSource(3)">100M Ethernet</div>
      </div>
    </div>

    <div class="card">
      <h2>ViPER4Android & &Phi;-Harmonic DSP Architecture</h2>
      <div class="ctrl-row">
        <div class="switch-wrap"><span>Dynamic Bass</span><div class="toggle active" id="tgBass" onclick="toggleDSP('bass')"></div></div>
        <input type="range" id="slBass" min="0" max="100" value="60" oninput="updateDSP()" />
        <span id="valBass">60%</span>
      </div>
      <div class="ctrl-row">
        <div class="switch-wrap"><span style="color:var(--accent-phi);">&Phi; Tube Exciter</span><div class="toggle active" id="tgExciter" onclick="toggleDSP('exciter')"></div></div>
        <input type="range" id="slExciter" min="0" max="100" value="45" oninput="updateDSP()" />
        <span id="valExciter">45%</span>
      </div>
      <div class="ctrl-row">
        <div class="switch-wrap"><span>Convolver (IRS)</span><div class="toggle active" id="tgConv" onclick="toggleDSP('conv')"></div></div>
        <input type="range" id="slConv" min="0" max="100" value="35" oninput="updateDSP()" />
        <span id="valConv">35%</span>
      </div>
      <div class="ctrl-row">
        <div class="switch-wrap"><span>Golden Q EQ</span><div class="toggle active" id="tgEq" onclick="toggleDSP('eq')"></div></div>
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
      <h2>IRS Acoustic Profile Drag & Drop Uploader</h2>
      <div class="drop-zone" id="dropZone">
        Drop custom <strong>.irs / .wav</strong> acoustic impulse file here or click to browse
        <input type="file" id="fileIrs" accept=".irs,.wav" style="display:none;" onchange="uploadIRS(this.files[0])" />
      </div>
    </div>

    <div class="card">
      <h2>Browser OTA Firmware Update</h2>
      <div class="ctrl-row">
        <span>Firmware Binary:</span>
        <input type="file" id="fileOta" accept=".bin" />
        <button style="background:#0284c7; color:#fff; border:none; padding:6px 12px; border-radius:4px; cursor:pointer;" onclick="uploadOTA()">Flash OTA</button>
      </div>
      <div id="otaProgress" style="font-size:10px; color:var(--text-muted); font-family:monospace;"></div>
    </div>

    <footer>
      &copy; 2026 Alex (<a href="https://github.com/kniajevich" target="_blank">@kniajevich</a>). All Rights Reserved.<br/>
      Contact: <a href="mailto:kniajevich@users.noreply.github.com">kniajevich@users.noreply.github.com</a><br/>
      Licensed under GNU AGPLv3. Dual-Licensing Available for Commercial Production.
    </footer>
  </div>

  <script>
    let dspState = { bass: true, bassVal: 0.6, exciter: true, exciterVal: 0.45, conv: true, convVal: 0.35, eq: true, eqVal: 2.0, vol: 0.85, source: 0 };

    function setSource(idx) {
      dspState.source = idx;
      document.querySelectorAll('.src-btn').forEach((b, i) => b.classList.toggle('active', i === idx));
      sendState();
    }

    function toggleDSP(param) {
      dspState[param] = !dspState[param];
      const ids = { bass: 'tgBass', exciter: 'tgExciter', conv: 'tgConv', eq: 'tgEq' };
      document.getElementById(ids[param]).classList.toggle('active', dspState[param]);
      sendState();
    }

    function updateDSP() {
      dspState.bassVal = parseInt(document.getElementById('slBass').value) / 100.0;
      document.getElementById('valBass').innerText = `${Math.round(dspState.bassVal * 100)}%`;
      dspState.exciterVal = parseInt(document.getElementById('slExciter').value) / 100.0;
      document.getElementById('valExciter').innerText = `${Math.round(dspState.exciterVal * 100)}%`;
      dspState.convVal = parseInt(document.getElementById('slConv').value) / 100.0;
      document.getElementById('valConv').innerText = `${Math.round(dspState.convVal * 100)}%`;
      dspState.eqVal = parseFloat(document.getElementById('slEq').value);
      document.getElementById('valEq').innerText = `${dspState.eqVal > 0 ? '+' : ''}${dspState.eqVal} dB`;
      dspState.vol = parseInt(document.getElementById('slVol').value) / 100.0;
      document.getElementById('valVol').innerText = `${Math.round(dspState.vol * 100)}%`;
      sendState();
    }

    function sendState() {
      fetch('/api/dsp', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(dspState) }).catch(() => {});
    }

    // Live VU polling
    setInterval(() => {
      fetch('/api/vu').then(r => r.json()).then(data => {
        document.getElementById('vuL').style.width = `${Math.min(100, data.l * 120)}%`;
        document.getElementById('vuR').style.width = `${Math.min(100, data.r * 120)}%`;
      }).catch(() => {});
    }, 100);

    // Drag & Drop IRS uploader
    const dz = document.getElementById('dropZone');
    dz.onclick = () => document.getElementById('fileIrs').click();
    dz.ondragover = (e) => { e.preventDefault(); dz.classList.add('hover'); };
    dz.ondragleave = () => dz.classList.remove('hover');
    dz.ondrop = (e) => {
      e.preventDefault();
      dz.classList.remove('hover');
      if (e.dataTransfer.files.length) uploadIRS(e.dataTransfer.files[0]);
    };

    function uploadIRS(file) {
      if (!file) return;
      dz.innerText = `Uploading ${file.name}...`;
      fetch('/api/upload-irs', { method: 'POST', body: file })
        .then(r => r.text())
        .then(() => { dz.innerHTML = `Loaded: <strong>${file.name}</strong> into PSRAM!`; })
        .catch(() => { dz.innerText = 'Upload failed!'; });
    }

    function uploadOTA() {
      const f = document.getElementById('fileOta').files[0];
      if (!f) return alert('Select .bin file');
      const p = document.getElementById('otaProgress');
      p.innerText = 'Flashing firmware to OTA partition...';
      fetch('/api/ota', { method: 'POST', body: f })
        .then(r => r.text())
        .then(() => { p.innerText = 'Flash complete! Device rebooting...'; })
        .catch(() => { p.innerText = 'OTA failed!'; });
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

    ESP_LOGI(TAG, "Starting HTTP Web Server on port %u...", port);
    if (httpd_start(&m_server, &config) != ESP_OK) return ESP_FAIL;

    httpd_uri_t root_uri = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };
    httpd_register_uri_handler(m_server, &root_uri);

    httpd_uri_t vu_uri = { .uri = "/api/vu", .method = HTTP_GET, .handler = vu_get_handler };
    httpd_register_uri_handler(m_server, &vu_uri);

    httpd_uri_t ota_uri = { .uri = "/api/ota", .method = HTTP_POST, .handler = ota_post_handler };
    httpd_register_uri_handler(m_server, &ota_uri);

    httpd_uri_t irs_uri = { .uri = "/api/upload-irs", .method = HTTP_POST, .handler = upload_irs_handler };
    httpd_register_uri_handler(m_server, &irs_uri);

    return ESP_OK;
}

void WebServer::stop() {
    if (m_server) {
        httpd_stop(m_server);
        m_server = nullptr;
    }
}
