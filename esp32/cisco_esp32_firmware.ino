/*
 ======================================================================================
   CISCO PACKET TRACER PHYSICAL PROTOTYPE - STANDALONE MASTER FIRMWARE
   WS2812B NeoPixel LED Strip Controller + Built-in Web Server + Hardware Setup
 ======================================================================================
   Ishlash tartibi:
   1. ESP32 o'zining shaxsiy Wi-Fi tarmog'ini ochadi:
      - SSID: Cisco_Maket_AP
      - Parol: 12345678
      - Manzil: http://192.168.4.1 (yoki avtomatik ochiladi)
   2. Shuningdek, telefon Wi-Fi tarmog'iga ham ulanadi (Xiaomi 12 Lite / insurgent)
   3. Web-sahifa to'liq ESP32 ichida joylashgan:
      - Hech qanday tashqi internet shart emas!
      - 0 ms kechikish: Tugma bosilishi bilan optik nur darhol lentada yuguradi!
      - Klient sahifasi + Paket yuborish + Har bir LEDni tekshirish + NVS Sozlamalari!
 ======================================================================================
*/

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

// Wi-Fi Access Point sozlamalari
const char *ap_ssid = "Cisco_Maket_AP";
const char *ap_password = "12345678";

// Qo'shimcha Wi-Fi (telefon hotspoti)
const char *sta_ssid = "Xiaomi 12 Lite";
const char *sta_password = "insurgent";

#define DEFAULT_PIN 4
#define DEFAULT_TOTAL_LEDS 60
#define DEFAULT_BRIGHTNESS 120

struct Config {
  int pin = DEFAULT_PIN;
  int total = DEFAULT_TOTAL_LEDS;
  int brightness = DEFAULT_BRIGHTNESS;
  int wan_start = 0;
  int wan_end = 11;
  int r_sw_start = 12;
  int r_sw_end = 19;
  int pc1_start = 20;
  int pc1_end = 31;
  int pc2_start = 32;
  int pc2_end = 43;
  int srv_start = 44;
  int srv_end = 55;
} cfg;

Preferences prefs;
WebServer server(80);
DNSServer dnsServer;
Adafruit_NeoPixel *strip = nullptr;

// ======================================================================================
// O'RNATILGAN VEB-INTERFEYS (HTML, CSS, JS TO'LIQ ESP32 XOTIRASIDA)
// ======================================================================================
const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uz">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Cisco Optical Maket Controller</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, monospace; }
    body { background: #080c14; color: #f1f5f9; padding: 12px; }
    .header { text-align: center; padding: 14px 0; border-bottom: 1px solid #1e293b; margin-bottom: 16px; }
    .header h1 { font-size: 19px; color: #00ffcc; letter-spacing: 1px; display: flex; align-items: center; justify-content: center; gap: 8px; }
    .header p { font-size: 11px; color: #94a3b8; margin-top: 4px; }
    .card { background: #0f172a; border: 1px solid #1e293b; border-radius: 14px; padding: 14px; margin-bottom: 14px; box-shadow: 0 4px 15px rgba(0,0,0,0.4); }
    .card-title { font-size: 13px; font-weight: bold; color: #38bdf8; text-transform: uppercase; margin-bottom: 12px; display: flex; align-items: center; justify-content: space-between; }
    .form-group { margin-bottom: 12px; }
    label { display: block; font-size: 11px; color: #cbd5e1; margin-bottom: 5px; font-weight: 600; text-transform: uppercase; }
    input[type="text"], input[type="number"], select { width: 100%; background: #020617; border: 1px solid #334155; border-radius: 9px; padding: 10px; color: #fff; font-size: 14px; outline: none; }
    input:focus, select:focus { border-color: #00ffcc; }
    .color-row { display: flex; gap: 8px; align-items: center; }
    .color-btn { width: 34px; height: 34px; border-radius: 50%; border: 2px solid transparent; cursor: pointer; }
    .color-btn.active { border-color: #fff; transform: scale(1.1); box-shadow: 0 0 10px rgba(255,255,255,0.5); }
    .send-btn { width: 100%; background: linear-gradient(135deg, #00ffcc 0%, #0284c7 100%); color: #020617; font-size: 15px; font-weight: bold; padding: 13px; border: none; border-radius: 11px; cursor: pointer; text-transform: uppercase; letter-spacing: 1px; box-shadow: 0 0 15px rgba(0,255,204,0.3); transition: all 0.2s; }
    .send-btn:active { transform: scale(0.97); }
    .btn-row { display: flex; gap: 8px; }
    .btn-sub { flex: 1; padding: 9px; background: #1e293b; color: #e2e8f0; border: 1px solid #334155; border-radius: 8px; font-size: 11px; font-weight: bold; cursor: pointer; text-align: center; }
    .btn-sub:hover, .btn-sub:active { background: #334155; }
    .status-box { background: #022c22; border: 1px solid #059669; color: #6ee7b7; padding: 10px; border-radius: 9px; font-size: 12px; margin-top: 10px; display: none; }
    .grid-leds { display: grid; grid-template-columns: repeat(12, 1fr); gap: 4px; max-height: 120px; overflow-y: auto; background: #020617; padding: 6px; border-radius: 8px; border: 1px solid #1e293b; }
    .grid-leds button { background: #0f172a; border: 1px solid #334155; color: #94a3b8; font-size: 9px; padding: 5px 0; border-radius: 4px; cursor: pointer; }
    .grid-leds button.active { background: #00ffcc; color: #000; font-weight: bold; border-color: #fff; }
    .tabs { display: flex; gap: 6px; margin-bottom: 12px; }
    .tab { flex: 1; padding: 9px; background: #1e293b; color: #94a3b8; border: none; border-radius: 8px; font-size: 12px; font-weight: bold; cursor: pointer; text-align: center; }
    .tab.active { background: #0284c7; color: #fff; }
    .range-row { display: flex; align-items: center; justify-content: space-between; font-size: 11px; color: #94a3b8; margin-bottom: 6px; }
    .range-inputs { display: flex; gap: 6px; align-items: center; }
    .range-inputs input { width: 55px; padding: 5px; text-align: center; }
  </style>
</head>
<body>

  <div class="header">
    <h1>⚡ CISCO OPTICAL PROTOTYPE</h1>
    <p>ESP32 Standalone Web Controller &bull; Kechikish: 0ms</p>
  </div>

  <div class="tabs">
    <button class="tab active" onclick="switchTab('tabSend')">🚀 Paket Yuborish</button>
    <button class="tab" onclick="switchTab('tabInspect')">🔬 LED Sinov</button>
    <button class="tab" onclick="switchTab('tabSetup')">⚙️ Sozlamalar</button>
  </div>

  <!-- TAB 1: PAKET YUBORISH -->
  <div id="tabSend">
    <div class="card">
      <div class="card-title">Optik Paket Parametrlari</div>
      
      <div class="form-group">
        <label>Yuboruvchi Ismi / Tuxallusi:</label>
        <input type="text" id="senderName" value="Talaba-Admin" placeholder="Ismingizni yozing">
      </div>

      <div class="form-group">
        <label>Qabul Qiluvchi Qurilma (Target IP):</label>
        <select id="targetSelect">
          <option value="PC1">💻 PC1 (192.168.1.10)</option>
          <option value="PC2">💻 PC2 (192.168.1.20)</option>
          <option value="Server0">🖥️ Server0 (192.168.1.100)</option>
        </select>
      </div>

      <div class="form-group">
        <label>Optik Nur Rangi:</label>
        <div class="color-row">
          <div class="color-btn active" style="background:#00ffcc" onclick="pickColor('#00ffcc', this)"></div>
          <div class="color-btn" style="background:#00ff66" onclick="pickColor('#00ff66', this)"></div>
          <div class="color-btn" style="background:#0088ff" onclick="pickColor('#0088ff', this)"></div>
          <div class="color-btn" style="background:#ff007f" onclick="pickColor('#ff007f', this)"></div>
          <div class="color-btn" style="background:#ffaa00" onclick="pickColor('#ffaa00', this)"></div>
          <input type="color" id="customColor" value="#00ffcc" style="width:34px; height:34px; border:none; background:transparent; cursor:pointer;" onchange="pickColor(this.value)">
        </div>
      </div>

      <button class="send-btn" onclick="sendOpticalPacket()">Optik Toladan Yuborish ➔</button>

      <div id="statusBox" class="status-box"></div>
    </div>
  </div>

  <!-- TAB 2: LED INSPECTOR -->
  <div id="tabInspect" style="display:none;">
    <div class="card">
      <div class="card-title">
        <span>Har Bir LEDni Alohida Sinash</span>
        <span id="inspectBadge" style="color:#00ffcc;">LED #0</span>
      </div>

      <div class="btn-row" style="margin-bottom:10px;">
        <button class="btn-sub" onclick="stepLed(-1)">◀ Oldingi</button>
        <button class="btn-sub" style="background:#0284c7;" onclick="stepLed(1)">Keyingi ▶</button>
        <button class="btn-sub" id="sweepBtn" style="background:#7c3aed;" onclick="toggleAutoSweep()">🔄 Avto-Skan</button>
      </div>

      <div class="btn-row" style="margin-bottom:12px;">
        <button class="btn-sub" style="background:#854d0e;" onclick="testAllOn()">💡 Barchasini Yoqish (All ON)</button>
        <button class="btn-sub" onclick="clearLeds()">⚫ O'chirish</button>
      </div>

      <label style="margin-top:10px;">Lenta Xaritasi (Bosib yoqing):</label>
      <div id="ledGrid" class="grid-leds"></div>
    </div>
  </div>

  <!-- TAB 3: SETUP -->
  <div id="tabSetup" style="display:none;">
    <div class="card">
      <div class="card-title">Plata va Segmentlar Sozlamalari</div>
      
      <div class="form-group">
        <label>LED Data Pini (GPIO):</label>
        <select id="cfgPin">
          <option value="4">GPIO 4 (D4)</option>
          <option value="2">GPIO 2 (D2)</option>
          <option value="5">GPIO 5 (D5)</option>
          <option value="18">GPIO 18 (D18)</option>
          <option value="19">GPIO 19 (D19)</option>
          <option value="23">GPIO 23 (D23)</option>
        </select>
      </div>

      <div class="form-group">
        <label>Jami LED Soni:</label>
        <input type="number" id="cfgTotal" value="60">
      </div>

      <div class="form-group">
        <label>Yorqinlik (10 - 255):</label>
        <input type="number" id="cfgBrightness" value="120" min="10" max="255">
      </div>

      <div class="range-row"><span>WAN (Optik Tola):</span><div class="range-inputs"><input type="number" id="wan_s" value="0">-<input type="number" id="wan_e" value="11"></div></div>
      <div class="range-row"><span>Router ➔ Switch:</span><div class="range-inputs"><input type="number" id="r_sw_s" value="12">-<input type="number" id="r_sw_e" value="19"></div></div>
      <div class="range-row"><span>Switch ➔ PC1:</span><div class="range-inputs"><input type="number" id="pc1_s" value="20">-<input type="number" id="pc1_e" value="31"></div></div>
      <div class="range-row"><span>Switch ➔ PC2:</span><div class="range-inputs"><input type="number" id="pc2_s" value="32">-<input type="number" id="pc2_e" value="43"></div></div>
      <div class="range-row"><span>Switch ➔ Server:</span><div class="range-inputs"><input type="number" id="srv_s" value="44">-<input type="number" id="srv_e" value="55"></div></div>

      <button class="send-btn" style="margin-top:12px; background:#059669;" onclick="saveConfig()">Xotiraga Saqlash (Save NVS)</button>
    </div>
  </div>

  <script>
    let selectedColor = '#00ffcc';
    let currentLed = 0;
    let totalLeds = 60;
    let sweepTimer = null;

    function switchTab(id) {
      document.getElementById('tabSend').style.display = (id === 'tabSend') ? 'block' : 'none';
      document.getElementById('tabInspect').style.display = (id === 'tabInspect') ? 'block' : 'none';
      document.getElementById('tabSetup').style.display = (id === 'tabSetup') ? 'block' : 'none';
      const tabs = document.querySelectorAll('.tab');
      tabs.forEach((t, i) => {
        t.className = (['tabSend','tabInspect','tabSetup'][i] === id) ? 'tab active' : 'tab';
      });
      if (id === 'tabInspect') buildGrid();
    }

    function pickColor(color, el) {
      selectedColor = color;
      if (el) {
        document.querySelectorAll('.color-btn').forEach(b => b.classList.remove('active'));
        el.classList.add('active');
      }
    }

    function sendOpticalPacket() {
      const sender = document.getElementById('senderName').value || 'Talaba';
      const target = document.getElementById('targetSelect').value;
      const statusBox = document.getElementById('statusBox');

      statusBox.style.display = 'block';
      statusBox.innerHTML = `⚡ <strong>${sender}</strong> tomonidan <strong>${target}</strong> ga nur yuborildi!`;

      fetch(`/sendWan?target=${target}&color=${encodeURIComponent(selectedColor)}`)
        .then(r => r.json())
        .catch(() => {});
    }

    function buildGrid() {
      const grid = document.getElementById('ledGrid');
      grid.innerHTML = '';
      for (let i = 0; i < totalLeds; i++) {
        const btn = document.createElement('button');
        btn.textContent = i;
        btn.className = (i === currentLed) ? 'active' : '';
        btn.onclick = () => { currentLed = i; sendSingleLed(i); };
        grid.appendChild(btn);
      }
    }

    function sendSingleLed(idx) {
      document.getElementById('inspectBadge').textContent = 'LED #' + idx;
      buildGrid();
      fetch(`/testLed?index=${idx}&color=${encodeURIComponent(selectedColor)}`);
    }

    function stepLed(dir) {
      currentLed += dir;
      if (currentLed < 0) currentLed = totalLeds - 1;
      if (currentLed >= totalLeds) currentLed = 0;
      sendSingleLed(currentLed);
    }

    function toggleAutoSweep() {
      const btn = document.getElementById('sweepBtn');
      if (sweepTimer) {
        clearInterval(sweepTimer);
        sweepTimer = null;
        btn.textContent = '🔄 Avto-Skan';
        btn.style.background = '#7c3aed';
      } else {
        btn.textContent = '⏹ To\'xtatish';
        btn.style.background = '#e11d48';
        sweepTimer = setInterval(() => { stepLed(1); }, 150);
      }
    }

    function testAllOn() {
      fetch(`/testAll?color=${encodeURIComponent(selectedColor)}`);
    }

    function clearLeds() {
      fetch('/clear');
    }

    function saveConfig() {
      const p = new URLSearchParams({
        pin: document.getElementById('cfgPin').value,
        total: document.getElementById('cfgTotal').value,
        brightness: document.getElementById('cfgBrightness').value,
        wan_start: document.getElementById('wan_s').value,
        wan_end: document.getElementById('wan_e').value,
        r_sw_start: document.getElementById('r_sw_s').value,
        r_sw_end: document.getElementById('r_sw_e').value,
        pc1_start: document.getElementById('pc1_s').value,
        pc1_end: document.getElementById('pc1_e').value,
        pc2_start: document.getElementById('pc2_s').value,
        pc2_end: document.getElementById('pc2_e').value,
        srv_start: document.getElementById('srv_s').value,
        srv_end: document.getElementById('srv_e').value
      });
      fetch(`/saveConfig?${p.toString()}`)
        .then(() => alert('Sozlamalar saqlandi!'));
    }

    // Yuklanganda sozlamalarni ESP32 dan olish
    fetch('/getConfig')
      .then(r => r.json())
      .then(cfg => {
        if (cfg.pin) document.getElementById('cfgPin').value = cfg.pin;
        if (cfg.total) { totalLeds = cfg.total; document.getElementById('cfgTotal').value = cfg.total; }
        if (cfg.brightness) document.getElementById('cfgBrightness').value = cfg.brightness;
      }).catch(() => {});
  </script>
</body>
</html>
)rawliteral";

// ======================================================================================
// YORDAMCHI FUNKSIYALAR
// ======================================================================================

uint32_t parseHexColor(String hex) {
  hex.replace("#", "");
  hex.replace("%23", "");
  hex.trim();
  if (hex.length() < 6) return strip->Color(0, 255, 204);
  long num = strtol(hex.c_str(), NULL, 16);
  byte r = (num >> 16) & 0xFF;
  byte g = (num >> 8) & 0xFF;
  byte b = num & 0xFF;
  if (r == 0 && g == 0 && b == 0) return strip->Color(0, 255, 204);
  return strip->Color(r, g, b);
}

void initStrip() {
  if (strip != nullptr) delete strip;
  strip = new Adafruit_NeoPixel(cfg.total, cfg.pin, NEO_GRB + NEO_KHZ800);
  strip->begin();
  strip->setBrightness(cfg.brightness);
  strip->clear();
  strip->show();
}

void loadPreferences() {
  prefs.begin("cisco_cfg", true);
  cfg.pin = prefs.getInt("pin", DEFAULT_PIN);
  cfg.total = prefs.getInt("total", DEFAULT_TOTAL_LEDS);
  cfg.brightness = prefs.getInt("brightness", DEFAULT_BRIGHTNESS);
  cfg.wan_start = prefs.getInt("wan_s", 0);
  cfg.wan_end = prefs.getInt("wan_e", 11);
  cfg.r_sw_start = prefs.getInt("r_sw_s", 12);
  cfg.r_sw_end = prefs.getInt("r_sw_e", 19);
  cfg.pc1_start = prefs.getInt("pc1_s", 20);
  cfg.pc1_end = prefs.getInt("pc1_e", 31);
  cfg.pc2_start = prefs.getInt("pc2_s", 32);
  cfg.pc2_end = prefs.getInt("pc2_e", 43);
  cfg.srv_start = prefs.getInt("srv_s", 44);
  cfg.srv_end = prefs.getInt("srv_e", 55);
  prefs.end();
}

void savePreferences() {
  prefs.begin("cisco_cfg", false);
  prefs.putInt("pin", cfg.pin);
  prefs.putInt("total", cfg.total);
  prefs.putInt("brightness", cfg.brightness);
  prefs.putInt("wan_s", cfg.wan_start);
  prefs.putInt("wan_e", cfg.wan_end);
  prefs.putInt("r_sw_s", cfg.r_sw_start);
  prefs.putInt("r_sw_e", cfg.r_sw_end);
  prefs.putInt("pc1_s", cfg.pc1_start);
  prefs.putInt("pc1_e", cfg.pc1_end);
  prefs.putInt("pc2_s", cfg.pc2_start);
  prefs.putInt("pc2_e", cfg.pc2_end);
  prefs.putInt("srv_s", cfg.srv_start);
  prefs.putInt("srv_e", cfg.srv_end);
  prefs.end();
}

void animateSegment(int start, int end, uint32_t color, int delayMs = 30) {
  int step = (start <= end) ? 1 : -1;
  int cur = start;
  while (true) {
    strip->setPixelColor(cur, color);
    strip->show();
    delay(delayMs);
    strip->setPixelColor(cur, 0);
    strip->show();
    if (cur == end) break;
    cur += step;
  }
}

void blinkMonitor(int start, int end, uint32_t color, int blinks = 3) {
  for (int b = 0; b < blinks; b++) {
    for (int i = start; i <= end; i++) strip->setPixelColor(i, color);
    strip->show();
    delay(100);
    for (int i = start; i <= end; i++) strip->setPixelColor(i, 0);
    strip->show();
    delay(100);
  }
}

void triggerPacketAnimation(String target, uint32_t color) {
  Serial.printf("\n⚡ [NUR HARAKATI] Target: %s\n", target.c_str());

  // 1. WAN Optik kabel
  animateSegment(cfg.wan_start, cfg.wan_end, color, 25);

  // 2. Router -> Switch
  animateSegment(cfg.r_sw_start, cfg.r_sw_end, color, 25);

  // 3. Switch -> Target
  if (target.indexOf("PC1") >= 0) {
    int mon = max(cfg.pc1_start, cfg.pc1_end - 5);
    animateSegment(cfg.pc1_start, mon - 1, color, 25);
    blinkMonitor(mon, cfg.pc1_end, color, 4);
  } else if (target.indexOf("PC2") >= 0) {
    int mon = max(cfg.pc2_start, cfg.pc2_end - 5);
    animateSegment(cfg.pc2_start, mon - 1, color, 25);
    blinkMonitor(mon, cfg.pc2_end, color, 4);
  } else if (target.indexOf("Server") >= 0) {
    int mon = max(cfg.srv_start, cfg.srv_end - 5);
    animateSegment(cfg.srv_start, mon - 1, color, 25);
    blinkMonitor(mon, cfg.srv_end, color, 4);
  } else {
    int mon = max(cfg.pc1_start, cfg.pc1_end - 5);
    animateSegment(cfg.pc1_start, mon - 1, color, 25);
    blinkMonitor(mon, cfg.pc1_end, color, 3);
  }

  strip->clear();
  strip->show();
}

// ======================================================================================
// SERVER HANDLERS
// ======================================================================================
void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send_P(200, "text/html", PAGE_INDEX);
}

void handleSendWan() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String target = server.hasArg("target") ? server.arg("target") : "PC1";
  String colorHex = server.hasArg("color") ? server.arg("color") : "#00ffcc";
  uint32_t color = parseHexColor(colorHex);

  server.send(200, "application/json", "{\"success\":true,\"target\":\"" + target + "\"}");
  triggerPacketAnimation(target, color);
}

void handleTestLed() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  int idx = server.hasArg("index") ? server.arg("index").toInt() : 0;
  String colorHex = server.hasArg("color") ? server.arg("color") : "#00ffcc";
  uint32_t color = parseHexColor(colorHex);

  if (idx >= 0 && idx < cfg.total) {
    strip->clear();
    strip->setPixelColor(idx, color);
    strip->show();
    Serial.printf("[TEST] LED #%d yondi\n", idx);
  }
  server.send(200, "application/json", "{\"success\":true,\"index\":" + String(idx) + "}");
}

void handleTestAll() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String colorHex = server.hasArg("color") ? server.arg("color") : "#ffffff";
  uint32_t color = parseHexColor(colorHex);

  for (int i = 0; i < cfg.total; i++) strip->setPixelColor(i, color);
  strip->show();
  Serial.println("[TEST-ALL] Barcha LEDlar yondi!");
  server.send(200, "application/json", "{\"success\":true}");
}

void handleClear() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  strip->clear();
  strip->show();
  server.send(200, "application/json", "{\"success\":true}");
}

void handleGetConfig() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String json = "{";
  json += "\"pin\":" + String(cfg.pin) + ",";
  json += "\"total\":" + String(cfg.total) + ",";
  json += "\"brightness\":" + String(cfg.brightness) + ",";
  json += "\"wan_start\":" + String(cfg.wan_start) + ",";
  json += "\"wan_end\":" + String(cfg.wan_end) + ",";
  json += "\"r_sw_start\":" + String(cfg.r_sw_start) + ",";
  json += "\"r_sw_end\":" + String(cfg.r_sw_end) + ",";
  json += "\"pc1_start\":" + String(cfg.pc1_start) + ",";
  json += "\"pc1_end\":" + String(cfg.pc1_end) + ",";
  json += "\"pc2_start\":" + String(cfg.pc2_start) + ",";
  json += "\"pc2_end\":" + String(cfg.pc2_end) + ",";
  json += "\"srv_start\":" + String(cfg.srv_start) + ",";
  json += "\"srv_end\":" + String(cfg.srv_end);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSaveConfig() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("pin")) cfg.pin = server.arg("pin").toInt();
  if (server.hasArg("total")) cfg.total = server.arg("total").toInt();
  if (server.hasArg("brightness")) cfg.brightness = server.arg("brightness").toInt();
  if (server.hasArg("wan_start")) cfg.wan_start = server.arg("wan_start").toInt();
  if (server.hasArg("wan_end")) cfg.wan_end = server.arg("wan_end").toInt();
  if (server.hasArg("r_sw_start")) cfg.r_sw_start = server.arg("r_sw_start").toInt();
  if (server.hasArg("r_sw_end")) cfg.r_sw_end = server.arg("r_sw_end").toInt();
  if (server.hasArg("pc1_start")) cfg.pc1_start = server.arg("pc1_start").toInt();
  if (server.hasArg("pc1_end")) cfg.pc1_end = server.arg("pc1_end").toInt();
  if (server.hasArg("pc2_start")) cfg.pc2_start = server.arg("pc2_start").toInt();
  if (server.hasArg("pc2_end")) cfg.pc2_end = server.arg("pc2_end").toInt();
  if (server.hasArg("srv_start")) cfg.srv_start = server.arg("srv_start").toInt();
  if (server.hasArg("srv_end")) cfg.srv_end = server.arg("srv_end").toInt();

  savePreferences();
  initStrip();
  server.send(200, "application/json", "{\"success\":true}");
}

// ======================================================================================
// SETUP & LOOP
// ======================================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n--- Cisco Packet Tracer Standalone Master Controller ---");
  loadPreferences();
  initStrip();

  // Boshlang'ich test: butun lenta zangori yonib o'chadi
  for (int i = 0; i < cfg.total; i++) strip->setPixelColor(i, strip->Color(0, 255, 204));
  strip->show();
  delay(500);
  strip->clear();
  strip->show();

  // 1. Dual Wi-Fi rejimini yoqish
  WiFi.mode(WIFI_AP_STA);

  // AP Wi-Fi ochish (har doim kafolatli ishlaydi!)
  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("\n========================================================");
  Serial.print("✅ ESP32 Wi-Fi AP ochildi: ");
  Serial.println(ap_ssid);
  Serial.println("🌐 Sayt manzili: http://192.168.4.1");
  Serial.println("========================================================\n");

  // Captive Portal DNS server (har qanday so'rovni 192.168.4.1 ga yo'naltirish)
  dnsServer.start(53, "*", local_ip);

  // Telefon hotspoti bo'lsa unga ham ulanish
  WiFi.begin(sta_ssid, sta_password);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 6) {
    delay(400);
    t++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("✅ Telefon Hotspotiga ulandi! IP: ");
    Serial.println(WiFi.localIP());
  }

  // WebServer marshrutlari
  server.on("/", HTTP_GET, handleRoot);
  server.on("/sendWan", HTTP_GET, handleSendWan);
  server.on("/testLed", HTTP_GET, handleTestLed);
  server.on("/testAll", HTTP_GET, handleTestAll);
  server.on("/clear", HTTP_GET, handleClear);
  server.on("/getConfig", HTTP_GET, handleGetConfig);
  server.on("/saveConfig", HTTP_GET, handleSaveConfig);

  // Android va iPhone uchun Captive Portal avto-yo'naltirish
  server.on("/generate_204", HTTP_GET, handleRoot);
  server.on("/hotspot-detect.html", HTTP_GET, handleRoot);

  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
  Serial.println("🚀 WebServer muvaffaqiyatli ishga tushdi!");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}
