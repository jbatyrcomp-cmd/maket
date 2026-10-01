/*
 ======================================================================================
   CISCO PACKET TRACER PHYSICAL PROTOTYPE - ESP32 INTERNET EDITION
   WS2812B LED Controller + Direct Vercel Cloud Polling + Local WebServer
 ======================================================================================
   Xususiyatlari:
   1. Telefon Wi-Fi tarmog'iga ulanish:
      - SSID: "Xiaomi 12 Lite"
      - Parol: "insurgent"
      - Zaxira Wi-Fi AP: "Cisco_Maket_AP" (192.168.4.1)
   2. To'g'ridan-to'g'ri Vercel Bulutiga ulanish:
      - Har 1 soniyada https://maket-lovat.vercel.app/api/packets dan paketlarni o'zi oladi!
      - Hech qanday admin ko'prigi shart emas — maket to'liq avtonom ishlaydi!
   3. Mahalliy WebServer API (/sendWan, /clear, /testLed, /testAll, /getConfig, /saveConfig)
 ======================================================================================
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

// 1. Wi-Fi STA sozlamalari (Telefon Hotspoti)
const char *sta_ssid = "Xiaomi 12 Lite";
const char *sta_password = "insurgent";

// 2. Wi-Fi AP sozlamalari (Zaxira ulanish)
const char *ap_ssid = "Cisco_Maket_AP";
const char *ap_password = "12345678";

// 3. Vercel Bulut manzili
const char *vercel_url = "https://maket-lovat.vercel.app/api/packets";

// Standart parametrlar
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
Adafruit_NeoPixel *strip = nullptr;

unsigned long lastCloudCheck = 0;
const unsigned long CLOUD_INTERVAL = 1000; // Har 1 soniyada Vercelni tekshirish

// Rang yordamchi funksiyasi (HEX string -> uint32_t)
uint32_t parseHexColor(String hex) {
  hex.replace("#", "");
  hex.replace("%23", "");
  hex.trim();
  if (hex.length() < 6) {
    return strip->Color(0, 255, 204); // Default Cyan
  }
  long number = strtol(hex.c_str(), NULL, 16);
  byte r = (number >> 16) & 0xFF;
  byte g = (number >> 8) & 0xFF;
  byte b = number & 0xFF;
  if (r == 0 && g == 0 && b == 0) {
    return strip->Color(0, 255, 204);
  }
  return strip->Color(r, g, b);
}

// LED lentani initsializatsiya qilish
void initStrip() {
  if (strip != nullptr) {
    delete strip;
  }
  strip = new Adafruit_NeoPixel(cfg.total, cfg.pin, NEO_GRB + NEO_KHZ800);
  strip->begin();
  strip->setBrightness(cfg.brightness);
  strip->clear();
  strip->show();
}

// NVS sozlamalarini yuklash
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

// NVS ga saqlash
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

void sendCorsHeader() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// Lenta segmenti bo'ylab nur o'tishi
void animateSegment(int start, int end, uint32_t color, int delayMs = 30) {
  int step = (start <= end) ? 1 : -1;
  int current = start;

  while (true) {
    strip->setPixelColor(current, color);
    strip->show();
    delay(delayMs);
    strip->setPixelColor(current, 0);
    strip->show();

    if (current == end) break;
    current += step;
  }
}

// Monitor LEDlari miltillashi
void blinkMonitor(int monitorStart, int monitorEnd, uint32_t color, int blinks = 3) {
  for (int b = 0; b < blinks; b++) {
    for (int i = monitorStart; i <= monitorEnd; i++) {
      strip->setPixelColor(i, color);
    }
    strip->show();
    delay(100);
    for (int i = monitorStart; i <= monitorEnd; i++) {
      strip->setPixelColor(i, 0);
    }
    strip->show();
    delay(100);
  }
}

// Paket harakatini lentada to'liq ko'rsatish
void triggerPacketAnimation(String target, uint32_t color) {
  Serial.printf("\n⚡ [OPTICAL PULSE] Target: %s, Color: %X\n", target.c_str(), color);

  // 1. WAN Optik kabel
  animateSegment(cfg.wan_start, cfg.wan_end, color, 25);

  // 2. Router -> Switch
  animateSegment(cfg.r_sw_start, cfg.r_sw_end, color, 25);

  // 3. Switch -> Target qurilma
  if (target.indexOf("PC1") >= 0) {
    int monStart = max(cfg.pc1_start, cfg.pc1_end - 5);
    animateSegment(cfg.pc1_start, monStart - 1, color, 25);
    blinkMonitor(monStart, cfg.pc1_end, color, 4);
  } else if (target.indexOf("PC2") >= 0) {
    int monStart = max(cfg.pc2_start, cfg.pc2_end - 5);
    animateSegment(cfg.pc2_start, monStart - 1, color, 25);
    blinkMonitor(monStart, cfg.pc2_end, color, 4);
  } else if (target.indexOf("Server") >= 0) {
    int monStart = max(cfg.srv_start, cfg.srv_end - 5);
    animateSegment(cfg.srv_start, monStart - 1, color, 25);
    blinkMonitor(monStart, cfg.srv_end, color, 4);
  } else {
    // Standart PC1
    int monStart = max(cfg.pc1_start, cfg.pc1_end - 5);
    animateSegment(cfg.pc1_start, monStart - 1, color, 25);
    blinkMonitor(monStart, cfg.pc1_end, color, 3);
  }

  strip->clear();
  strip->show();
}

// ======================================================================================
// VERCEL BULUTIDAN TO'G'RIDAN-TO'G'RI O'QISH (Cloud Polling)
// ======================================================================================
void checkVercelQueue() {
  if (WiFi.status() != WL_CONNECTED) {
    return; // Internetga ulanmagan bo'lsa kutamiz
  }

  WiFiClientSecure client;
  client.setInsecure(); // SSL sertifikat tekshiruvini bypass qilish (tezkor)
  HTTPClient https;

  if (https.begin(client, vercel_url)) {
    https.setTimeout(3000);
    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
      String payload = https.getString();

      // Agar javobda "packets" bo'lsa va count > 0 bo'lsa
      if (payload.indexOf("\"count\":0") == -1 && payload.indexOf("\"packets\":[") >= 0) {
        Serial.println("\n🌐 [VERCEL CLOUD] Yangi paket qabul qilindi!");
        Serial.println(payload);

        // Oddiy JSON parsing (qurilma va rangni topish)
        int targetIdx = payload.indexOf("\"target\":\"");
        int colorIdx = payload.indexOf("\"color\":\"");

        String target = "PC1";
        String colorHex = "#00ffcc";

        if (targetIdx >= 0) {
          int start = targetIdx + 10;
          int end = payload.indexOf("\"", start);
          if (end > start) target = payload.substring(start, end);
        }

        if (colorIdx >= 0) {
          int start = colorIdx + 9;
          int end = payload.indexOf("\"", start);
          if (end > start) colorHex = payload.substring(start, end);
        }

        uint32_t color = parseHexColor(colorHex);
        triggerPacketAnimation(target, color);
      }
    }
    https.end();
  }
}

// ======================================================================================
// WEBSERVER HANDLERS
// ======================================================================================
void handlePing() {
  sendCorsHeader();
  server.send(200, "text/plain", "PONG");
}

void handleClear() {
  sendCorsHeader();
  strip->clear();
  strip->show();
  server.send(200, "application/json", "{\"success\":true}");
}

void handleSendWan() {
  sendCorsHeader();
  String target = server.hasArg("target") ? server.arg("target") : "PC1";
  String colorHex = server.hasArg("color") ? server.arg("color") : "#00ffcc";
  uint32_t color = parseHexColor(colorHex);

  server.send(200, "application/json", "{\"success\":true,\"target\":\"" + target + "\"}");
  triggerPacketAnimation(target, color);
}

void handleTestLed() {
  sendCorsHeader();
  int index = server.hasArg("index") ? server.arg("index").toInt() : 0;
  String colorHex = server.hasArg("color") ? server.arg("color") : "#00ffcc";
  uint32_t color = parseHexColor(colorHex);

  if (index >= 0 && index < cfg.total) {
    strip->clear();
    strip->setPixelColor(index, color);
    strip->show();
    Serial.printf("[TEST-LED] LED #%d yondi (%s)\n", index, colorHex.c_str());
  }
  server.send(200, "application/json", "{\"success\":true,\"index\":" + String(index) + "}");
}

void handleTestAll() {
  sendCorsHeader();
  String colorHex = server.hasArg("color") ? server.arg("color") : "#ffffff";
  uint32_t color = parseHexColor(colorHex);

  for (int i = 0; i < cfg.total; i++) {
    strip->setPixelColor(i, color);
  }
  strip->show();
  Serial.printf("[TEST-ALL] Barcha %d ta LED yondi\n", cfg.total);
  server.send(200, "application/json", "{\"success\":true}");
}

void handleGetConfig() {
  sendCorsHeader();
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
  json += "\"srv_end\":" + String(cfg.srv_end) + ",";
  json += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"ip\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleSaveConfig() {
  sendCorsHeader();
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

  Serial.println("\n--- Cisco Packet Tracer Physical Prototype (Internet Edition) ---");
  loadPreferences();
  initStrip();

  // Boshlang'ich test (chiroqlar 0.5 soniya yonib o'chadi)
  for (int i = 0; i < cfg.total; i++) {
    strip->setPixelColor(i, strip->Color(0, 255, 204));
  }
  strip->show();
  delay(500);
  strip->clear();
  strip->show();

  // 1. Dual Wi-Fi rejimini yoqish (ham telefon Hotspotiga ulanadi, ham zaxira AP ochadi)
  WiFi.mode(WIFI_AP_STA);

  // Zaxira AP ochish
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("Zaxira Wi-Fi AP: ");
  Serial.println(ap_ssid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Telefon Hotspotiga ("Xiaomi 12 Lite") ulanish
  Serial.printf("Internetga ulanilmoqda: '%s'...\n", sta_ssid);
  WiFi.begin(sta_ssid, sta_password);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 15) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ INTERNETGA MUVAFFAQIYATLI ULANDI!");
    Serial.print("ESP32 Lokal IP: ");
    Serial.println(WiFi.localIP());
    // Muvaffaqiyatli ulanish belgisi: 3 ta yashil chiroq yonadi
    blinkMonitor(0, min(5, cfg.total - 1), strip->Color(0, 255, 0), 2);
  } else {
    Serial.println("\n⚠️ Telefon Hotspotiga ulanib bo'lmadi. Telefoningizda 'Точка доступа' yoqilganligini tekshiring.");
  }

  // WebServer marshrutlari
  server.on("/ping", HTTP_GET, handlePing);
  server.on("/clear", HTTP_GET, handleClear);
  server.on("/sendWan", HTTP_GET, handleSendWan);
  server.on("/testLed", HTTP_GET, handleTestLed);
  server.on("/testAll", HTTP_GET, handleTestAll);
  server.on("/getConfig", HTTP_GET, handleGetConfig);
  server.on("/saveConfig", HTTP_GET, handleSaveConfig);

  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      sendCorsHeader();
      server.send(200);
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });

  server.begin();
  Serial.println("WebServer faol!");
}

void loop() {
  server.handleClient();

  // Har 1 soniyada Vercel bulut navbatini to'g'ridan-to'g'ri tekshirish
  if (millis() - lastCloudCheck >= CLOUD_INTERVAL) {
    lastCloudCheck = millis();
    checkVercelQueue();
  }
}
