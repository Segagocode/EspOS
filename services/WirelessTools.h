#ifndef ESPOS_WIRELESS_TOOLS_H
#define ESPOS_WIRELESS_TOOLS_H

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "../drivers/Display.h"
#include "../drivers/Touch.h"

class WirelessTools {
public:
  WirelessTools();

  // 1. WiFi Scanner
  static void scan(DisplayManager& display);

  // 2. Beacon Spam / Custom Beacon Transmit
  static void beaconSpam(const String& targetSSID, int count, DisplayManager& display, TouchManager& touch);

  // 3. Promiscuous Packet Monitor
  static void packetMonitor(int channel, DisplayManager& display, TouchManager& touch);

private:
  static void sendRawBeacon(const char* ssid, uint8_t channel);
};

#endif // ESPOS_WIRELESS_TOOLS_H
