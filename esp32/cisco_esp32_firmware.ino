/*
 ======================================================================================
   CISCO PACKET TRACER PHYSICAL PROTOTYPE - ESP32 FIRMWARE
   WS2812B / NeoPixel LED Strip Controller + WebServer
 ======================================================================================
   Xususiyatlari:
   - Wi-Fi Access Point (SSID: Cisco_Maket_AP, IP: 192.168.4.1)
   - WebServer API:
       * GET /sendWan?target=PC1&color=%2300ffcc -> Paket animatsiyasi
       * GET /clear                              -> Lentani tozalash
       * GET /ping                               -> Aloqani tekshirish
       * GET /getConfig                          -> NVS sozlamalarini JSON qaytarish
       * GET /saveConfig?...                     -> Sozlamalarni NVS ga saqlash
   - NVS (Preferences) orqali dinamik sozlanadigan GPIO pin, yorqinlik va segmentlar
 ======================================================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

// Wi-Fi Access Point sozlamalari
const char *ssid = "Cisco_Maket_AP";
const char *password = "12345678"; // kamida 8 ta belgi (yoki NULL ochiq qilish uchun)

// Standart parametrlar
#define DEFAULT_PIN 4
#define DEFAULT_TOTAL_LEDS 60
#define DEFAULT_BRIGHTNESS 120

// Segment oraliqlari (standart)
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

// Rang yordamchi funksiyasi (HEX string -> uint32_t)
uint32_t parseHexColor(String hex) {
  hex.replace("#", "");
  hex.replace("%23", "");
  hex.trim();
  if (hex.length() < 6) {
    return strip->Color(0, 255, 204); // Xatolik bo'lsa standart yorqin Cyan
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

// LED lentani qayta initsializatsiya qilish
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

// NVS xotirasidan sozlamalarni o'qish
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

// NVS xotirasiga sozlamalarni saqlash
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

// CORS javobini qo'shish
void sendCorsHeader() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// Bitta segment bo'ylab yorug'lik nurini (impuls) yurgizish animatsiyasi
void animateSegment(int start, int end, uint32_t color, int delayMs = 35) {
  int step = (start <= end) ? 1 : -1;
  int current = start;

  while (true) {
    strip->setPixelColor(current, color);
    strip->show();
    delay(delayMs);
    strip->setPixelColor(current, 0); // orqasidan o'chirish
    strip->show();

    if (current == end) break;
    current += step;
  }
}

// Monitor LEDlarini miltillatish (Paket qabul qilindi effekti)
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

// ======================================================================================
// SERVER HANDLERS
// ======================================================================================

// GET /ping
void handlePing() {
  sendCorsHeader();
  server.send(200, "text/plain", "PONG");
}

// GET /clear
void handleClear() {
  sendCorsHeader();
  strip->clear();
  strip->show();
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Cleared\"}");
}

// GET /sendWan?target=PC1&color=%2300ffcc
void handleSendWan() {
  sendCorsHeader();
  String target = server.hasArg("target") ? server.arg("target") : "PC1";
  String colorHex = server.hasArg("color") ? server.arg("color") : "#00ffcc";
  uint32_t color = parseHexColor(colorHex);

  Serial.printf("\n[BUYRUQ KELDI] /sendWan: Target=%s, Color=%s\n", target.c_str(), colorHex.c_str());

  // Darhol javob qaytaramiz (brauzer kutib qolmasligi uchun)
  server.send(200, "application/json", "{\"success\":true,\"target\":\"" + target + "\"}");

  // 1. WAN (Optik tola) segmentidan impuls o'tadi
  animateSegment(cfg.wan_start, cfg.wan_end, color, 30);

  // 2. Routerdan Switchga o'tadi
  animateSegment(cfg.r_sw_start, cfg.r_sw_end, color, 30);

  // 3. Switchdan Target qurilmaga o'tadi
  if (target == "PC1") {
    // Kabel bo'ylab yugurish
    int monStart = max(cfg.pc1_start, cfg.pc1_end - 5); // Oxirgi 6 ta LED monitor uchun
    animateSegment(cfg.pc1_start, monStart - 1, color, 30);
    // Monitor miltillashi
    blinkMonitor(monStart, cfg.pc1_end, color, 3);
  } else if (target == "PC2") {
    int monStart = max(cfg.pc2_start, cfg.pc2_end - 5);
    animateSegment(cfg.pc2_start, monStart - 1, color, 30);
    blinkMonitor(monStart, cfg.pc2_end, color, 3);
  } else if (target == "Server0" || target == "Server") {
    int monStart = max(cfg.srv_start, cfg.srv_end - 5);
    animateSegment(cfg.srv_start, monStart - 1, color, 30);
    blinkMonitor(monStart, cfg.srv_end, color, 3);
  } else {
    // Noma'lum qurilma bo'lsa barcha segmentlar bo'ylab o'chirish
    strip->clear();
    strip->show();
  }
}

// GET /getConfig
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
  json += "\"srv_end\":" + String(cfg.srv_end);
  json += "}";
  server.send(200, "application/json", json);
}

// GET /saveConfig?pin=...&...
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
  initStrip(); // Yangi parametrlar bilan lentani yangilash

  server.send(200, "application/json", "{\"success\":true,\"message\":\"Saved to NVS successfully\"}");
}

// ======================================================================================
// SETUP & LOOP
// ======================================================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n--- Cisco Packet Tracer Physical Prototype ---");
  loadPreferences();
  initStrip();

  // Wi-Fi AP yaratish
  WiFi.mode(WIFI_AP);
  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);

  Serial.print("Wi-Fi AP ochildi: ");
  Serial.println(ssid);
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.softAPIP());

  // WebServer marshrutlari
  server.on("/ping", HTTP_GET, handlePing);
  server.on("/clear", HTTP_GET, handleClear);
  server.on("/sendWan", HTTP_GET, handleSendWan);
  server.on("/getConfig", HTTP_GET, handleGetConfig);
  server.on("/saveConfig", HTTP_GET, handleSaveConfig);

  // Pre-flight OPTIONS CORS so'rovlari uchun
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      sendCorsHeader();
      server.send(200);
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  });

  server.begin();
  Serial.println("HTTP WebServer ishga tushdi!");
  Serial.printf("LED Pin: GPIO %d, Jami LED: %d, Yorqinlik: %d\n", cfg.pin, cfg.total, cfg.brightness);

  // Boshlang'ich TEST: Butun lenta 500ms ga chiroyli zangori yonib o'chadi
  for (int i = 0; i < cfg.total; i++) {
    strip->setPixelColor(i, strip->Color(0, 255, 204));
  }
  strip->show();
  delay(600);
  strip->clear();
  strip->show();
  Serial.println("Lenta self-test yakunlandi. ESP32 paket kutmoqda...");
}

void loop() {
  server.handleClient();
}
