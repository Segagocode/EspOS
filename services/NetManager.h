#ifndef ESPOS_NET_MANAGER_H
#define ESPOS_NET_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "../drivers/Display.h"

class EspNetManager {
public:
  EspNetManager();

  bool isConnected() const;
  String getSSID() const;
  String getIP() const;

  bool connect(const String& ssid, const String& pass, DisplayManager& display);
  void disconnect();
  void tryAutoConnect(DisplayManager& display);

  bool loadConfig(String& ssid, String& pass);
  void saveConfig(const String& ssid, const String& pass);

  void performOTA(const String& url, DisplayManager& display);
};

#endif // ESPOS_NET_MANAGER_H
