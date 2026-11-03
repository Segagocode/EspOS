#include "WirelessTools.h"
#include <esp_wifi.h>

WirelessTools::WirelessTools() {
}

// ====================== 1. SCAN ======================

void WirelessTools::scan(DisplayManager& display) {
  display.print("Scanning 2.4GHz WiFi...", COL_PROMPT);
  Serial.println("[Wireless] Starting WiFi scan...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks(false, true); // show hidden = true
  if (n <= 0) {
    display.print("No networks found", COL_WARN);
    return;
  }

  display.print("Found " + String(n) + " networks:", COL_OK);
  Serial.printf("[Wireless] Found %d networks\n", n);

  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) ssid = "<Hidden>";
    if (ssid.length() > 18) ssid = ssid.substring(0, 18);

    int32_t rssi = WiFi.RSSI(i);
    int32_t ch = WiFi.channel(i);
    wifi_auth_mode_t auth = WiFi.encryptionType(i);

    String authStr = "OPEN";
    if (auth == WIFI_AUTH_WEP) authStr = "WEP";
    else if (auth == WIFI_AUTH_WPA_PSK) authStr = "WPA";
    else if (auth == WIFI_AUTH_WPA2_PSK) authStr = "WPA2";
    else if (auth == WIFI_AUTH_WPA_WPA2_PSK) authStr = "WPA1/2";
    else if (auth == WIFI_AUTH_WPA2_ENTERPRISE) authStr = "ENT";
    else if (auth == WIFI_AUTH_WPA3_PSK) authStr = "WPA3";

    uint16_t col = COL_OK;
    if (rssi < -75) col = COL_ERROR;
    else if (rssi < -62) col = COL_DIR;

    char buf[42];
    snprintf(buf, sizeof(buf), "%-18s %2ddBm C%02d %s", ssid.c_str(), (int)rssi, (int)ch, authStr.c_str());
    display.print(String(buf), col);
    Serial.println(buf);
    delay(40);
  }

  WiFi.scanDelete();
}

// ====================== 2. BEACON SPAM ======================

static const char* defaultSpamList[] = {
  "EspOS_Marauder",
  "Never_Gonna_Give_You_Up",
  "FBI_Surveillance_Van",
  "Skynet_Defense_Net",
  "Virus_Installer.exe",
  "Free_Public_WiFi",
  "5G_Corona_Tower_#42",
  "Area51_Alien_Uplink",
  "Drop_It_Like_Its_Hot",
  "Totally_Not_A_Hacker"
};
static const int defaultSpamListSize = 10;

void WirelessTools::sendRawBeacon(const char* ssid, uint8_t channel) {
  uint8_t packet[128] = {
    0x80, 0x00, 0x00, 0x00,               // Type: Beacon
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   // Dest: Broadcast
    0x02, 0x11, 0x22, 0x33, 0x44, 0x55,   // Source MAC
    0x02, 0x11, 0x22, 0x33, 0x44, 0x55,   // BSSID
    0x00, 0x00,                           // Seq
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Timestamp
    0x64, 0x00,                           // Beacon Interval (100 TU)
    0x01, 0x04                            // Capabilities (ESS)
  };

  // Randomize Source MAC slightly
  packet[10] = (uint8_t)random(0, 255);
  packet[11] = (uint8_t)random(0, 255);
  packet[16] = packet[10];
  packet[17] = packet[11];

  int ssidLen = strlen(ssid);
  if (ssidLen > 32) ssidLen = 32;

  int idx = 36;
  packet[idx++] = 0x00;        // SSID Tag
  packet[idx++] = ssidLen;     // Tag Length
  memcpy(&packet[idx], ssid, ssidLen);
  idx += ssidLen;

  // Supported rates (1, 2, 5.5, 11 Mbps)
  uint8_t rates[] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c};
  memcpy(&packet[idx], rates, sizeof(rates));
  idx += sizeof(rates);

  // Channel parameter tag
  packet[idx++] = 0x03;        // DS Parameter Set
  packet[idx++] = 0x01;        // Length
  packet[idx++] = channel;     // Current Channel

  esp_wifi_80211_tx(WIFI_IF_AP, packet, idx, false);
}

void WirelessTools::beaconSpam(const String& targetSSID, int count, DisplayManager& display, TouchManager& touch) {
  display.print("Starting Beacon Spam...", COL_PROMPT);
  display.print("Tap screen or press key to stop", COL_WARN);

  WiFi.mode(WIFI_AP_STA);
  esp_wifi_set_promiscuous(false);

  int totalSent = 0;
  bool infinite = (count <= 0);
  int targetCount = infinite ? 999999 : count;

  while (totalSent < targetCount) {
    if (Serial.available()) {
      while (Serial.available()) Serial.read();
      break;
    }
    if (touch.isTouched()) {
      break;
    }

    uint8_t ch = (totalSent % 11) + 1;
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);

    if (targetSSID.length() > 0 && targetSSID != "spam") {
      sendRawBeacon(targetSSID.c_str(), ch);
      totalSent++;
    } else {
      for (int i = 0; i < defaultSpamListSize; i++) {
        sendRawBeacon(defaultSpamList[i], ch);
        totalSent++;
        delay(2);
      }
    }

    if (totalSent % 50 == 0) {
      display.print("Sent: " + String(totalSent) + " beacons (CH " + String(ch) + ")", COL_OK);
    }
    delay(10);
  }

  WiFi.mode(WIFI_STA);
  display.print("Beacon Spam stopped (" + String(totalSent) + " sent)", COL_TEXT);
}

// ====================== 3. PACKET MONITOR ======================

static volatile uint32_t g_pktCount = 0;
static volatile uint32_t g_deauthCount = 0;

static void IRAM_ATTR promiscuousRx(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA && type != WIFI_PKT_CTRL) return;
  g_pktCount = g_pktCount + 1;

  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  if (type == WIFI_PKT_MGMT && pkt->rx_ctrl.sig_len >= 24) {
    const uint8_t* payload = pkt->payload;
    uint16_t frameControl = payload[0] | (payload[1] << 8);
    // Subtype 0x00C0 = Deauthentication, 0x00A0 = Disassociation
    if ((frameControl & 0x00F0) == 0x00C0 || (frameControl & 0x00F0) == 0x00A0) {
      g_deauthCount = g_deauthCount + 1;
    }
  }
}

void WirelessTools::packetMonitor(int channel, DisplayManager& display, TouchManager& touch) {
  if (channel < 1 || channel > 14) channel = 1;

  Adafruit_ILI9341& tft = display.raw();
  display.setMode(DisplayMode::RAW_GRAPHICS);

  tft.fillScreen(COL_BG);
  tft.fillRect(0, 0, 240, 24, ILI9341_NAVY);
  tft.setTextColor(COL_PROMPT);
  tft.setTextSize(1);
  tft.setCursor(6, 8);
  tft.print("ESPOS PKTMON | CH: " + String(channel));

  tft.setTextColor(COL_TEXT);
  tft.setCursor(6, 30);
  tft.print("Tap screen to return to shell");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  g_pktCount = 0;
  g_deauthCount = 0;

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&promiscuousRx);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  const int graphX = 10;
  const int graphY = 80;
  const int graphW = 220;
  const int graphH = 180;

  tft.drawRect(graphX - 1, graphY - 1, graphW + 2, graphH + 2, COL_TEXT);

  int historyIdx = 0;

  unsigned long lastUpdate = millis();

  while (true) {
    if (Serial.available()) {
      while (Serial.available()) Serial.read();
      break;
    }
    if (touch.isTouched()) {
      break;
    }

    if (millis() - lastUpdate >= 250) {
      lastUpdate = millis();

      uint32_t currentPkts = g_pktCount * 4; // approximate pkts/sec
      g_pktCount = 0;
      uint32_t deauths = g_deauthCount;

      // Update Header stats
      tft.fillRect(0, 44, 240, 30, COL_BG);
      tft.setCursor(6, 46);
      tft.setTextColor(COL_OK);
      tft.print("Packets/s: " + String(currentPkts));

      tft.setCursor(140, 46);
      tft.setTextColor(deauths > 0 ? COL_ERROR : COL_TEXT);
      tft.print("Deauth: " + String(deauths));

      // Plot graph bar
      int barH = map(currentPkts, 0, 800, 0, graphH);
      if (barH > graphH) barH = graphH;

      // Shift graph left
      int colX = graphX + historyIdx;
      // Clear vertical line
      tft.drawFastVLine(colX, graphY, graphH, COL_BG);
      // Draw bar
      uint16_t barCol = (deauths > 0) ? COL_ERROR : (currentPkts > 300 ? COL_WARN : COL_OK);
      tft.drawFastVLine(colX, graphY + graphH - barH, barH, barCol);

      historyIdx = (historyIdx + 1) % graphW;
    }
    delay(20);
  }

  esp_wifi_set_promiscuous(false);
  display.setMode(DisplayMode::SHELL);
  display.clear();
  display.print("Packet Monitor stopped", COL_PROMPT);
  display.prompt("/");
}
