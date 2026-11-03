#include "NetManager.h"
#include <LittleFS.h>

EspNetManager::EspNetManager() {
}

bool EspNetManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String EspNetManager::getSSID() const {
  return WiFi.SSID();
}

String EspNetManager::getIP() const {
  return WiFi.localIP().toString();
}

bool EspNetManager::loadConfig(String& ssid, String& pass) {
  if (!LittleFS.exists("/config/wifi.cfg")) return false;
  File f = LittleFS.open("/config/wifi.cfg", "r");
  if (!f) return false;
  ssid = f.readStringUntil('\n');
  pass = f.readStringUntil('\n');
  ssid.trim();
  pass.trim();
  f.close();
  return ssid.length() > 0;
}

void EspNetManager::saveConfig(const String& ssid, const String& pass) {
  File f = LittleFS.open("/config/wifi.cfg", "w");
  if (!f) return;
  f.println(ssid);
  f.println(pass);
  f.close();
}

void EspNetManager::tryAutoConnect(DisplayManager& display) {
  String ssid, pass;
  if (!loadConfig(ssid, pass)) return;

  display.print("WiFi auto: " + ssid, COL_TEXT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.print("IP: " + WiFi.localIP().toString(), COL_OK);
  } else {
    display.print("WiFi auto failed", COL_ERROR);
  }
}

bool EspNetManager::connect(const String& ssid, const String& pass, DisplayManager& display) {
  display.print("Connecting: " + ssid, COL_TEXT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    display.print("Connected", COL_OK);
    display.print("IP: " + WiFi.localIP().toString(), COL_OK);
    saveConfig(ssid, pass);
    display.print("Saved to /config/wifi.cfg", COL_TEXT);
    return true;
  } else {
    display.print("Connect failed", COL_ERROR);
    return false;
  }
}

void EspNetManager::disconnect() {
  WiFi.disconnect(true);
}

static DisplayManager* g_otaDisplay = nullptr;

void EspNetManager::performOTA(const String& url, DisplayManager& display) {
  if (WiFi.status() != WL_CONNECTED) {
    display.print("WiFi not connected", COL_ERROR);
    return;
  }

  if (url.length() == 0) {
    display.print("Usage: update <url>", COL_ERROR);
    display.print("URL to .bin firmware", COL_TEXT);
    return;
  }

  display.setKeyboardVisible(false);
  display.clear();
  display.print("OTA Update...", COL_PROMPT);
  display.print(url, COL_TEXT);

  Adafruit_ILI9341& tft = display.raw();
  tft.drawRect(18, 140, 204, 24, COL_TEXT);
  tft.fillRect(20, 142, 0, 20, COL_OK);

  g_otaDisplay = &display;

  WiFiClientSecure client;
  client.setInsecure();

  httpUpdate.rebootOnUpdate(true);
  httpUpdate.onProgress([](int cur, int total) {
    if (total <= 0 || !g_otaDisplay) return;
    Adafruit_ILI9341& t = g_otaDisplay->raw();
    int pct = (cur * 100) / total;
    int bar = (pct * 200) / 100;
    t.fillRect(20, 142, bar, 20, COL_OK);
    t.fillRect(90, 170, 60, 14, COL_BG);
    t.setCursor(100, 172);
    t.setTextColor(COL_TEXT);
    t.setTextSize(1);
    t.print(String(pct) + "%");
  });

  t_httpUpdate_return ret = httpUpdate.update(client, url);

  if (ret == HTTP_UPDATE_FAILED) {
    display.print("OTA failed", COL_ERROR);
    display.print(httpUpdate.getLastErrorString(), COL_ERROR);
  } else if (ret == HTTP_UPDATE_NO_UPDATES) {
    display.print("No updates", COL_TEXT);
  }
}
