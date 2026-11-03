/*
 * EspOS - Minimal Modular OS for ESP32
 * v0.5.0-marauder: Single-file release for ArduinoDroid
 *
 * Generated automatically by tools/bundle.py
 * DO NOT EDIT DIRECTLY: edit modular files in config/, drivers/, services/, kernel/
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "esp_wifi.h"


// ============================================================
// FILE: config/Config.h
// ============================================================



// ====================== PINS ======================
// Display (ILI9341 SPI - Hardware VSPI)
#define TFT_CS          15
#define TFT_DC          2
#define TFT_RST         4
#define SPI_SCK         18
#define SPI_MOSI        23
#define SPI_MISO        19

// Touchscreen (XPT2046 SPI - Hardware HSPI, dedicated bus)
#define TOUCH_CS        33
#define TOUCH_CLK       25
#define TOUCH_MOSI      32
#define TOUCH_MISO      35
#define TOUCH_IRQ       27  // Hardware touch detector with internal INPUT_PULLUP

// ====================== UI CONFIG ======================
#define MAX_LINES       14
#define LINE_HEIGHT     12
#define MAX_LINE_LEN    38
#define INPUT_Y         172

// ====================== COLORS ======================
#define COL_BG          ILI9341_BLACK
#define COL_TEXT        ILI9341_WHITE
#define COL_PROMPT      ILI9341_CYAN
#define COL_ERROR       ILI9341_RED
#define COL_OK          ILI9341_GREEN
#define COL_DIR         ILI9341_YELLOW
#define COL_WARN        ILI9341_ORANGE
#define COL_MAGENTA     ILI9341_MAGENTA
#define COL_KBD         ILI9341_DARKGREY
#define COL_KBD_KEY     ILI9341_LIGHTGREY
#define COL_KBD_TEXT    ILI9341_BLACK
#define COL_BAR_BG      ILI9341_DARKGREY

// ====================== SYSTEM ======================
#define SERIAL_BAUD     115200
#define OS_NAME         "EspOS"
#define OS_VERSION      "0.5.0-marauder"


// ============================================================
// FILE: drivers/Display.h
// ============================================================



enum class DisplayMode {
  SHELL,
  STATUS,
  RAW_GRAPHICS
};

class DisplayManager {
public:
  DisplayManager();

  bool begin();
  void clear();
  void print(const String& text, uint16_t color = COL_TEXT);
  void prompt(const String& cwd);
  void drawInputLine(const String& inputBuffer);
  void drawKeyboard();
  void toggleKeyboard(const String& inputBuffer, const String& cwd);
  void redrawAll(const String& inputBuffer);

  bool isKeyboardVisible() const { return _kbdVisible; }
  void setKeyboardVisible(bool visible) { _kbdVisible = visible; }

  DisplayMode getMode() const { return _mode; }
  void setMode(DisplayMode mode) { _mode = mode; }

  Adafruit_ILI9341& raw() { return _tft; }

  // Keyboard layout constants
  static const char* kbdRows[5];
  static const int kbdRowLens[5];

private:
  Adafruit_ILI9341 _tft;
  DisplayMode _mode = DisplayMode::SHELL;
  bool _kbdVisible = false;

  String _lines[MAX_LINES];
  uint16_t _lineColors[MAX_LINES];
  int _lineCount = 0;

  void redrawTextArea();
};


// ============================================================
// FILE: drivers/Display.cpp
// ============================================================

#include "Display.h"

const char* DisplayManager::kbdRows[5] = {
  "1234567890",
  "qwertyuiop",
  "asdfghjkl/",
  "zxcvbnm._-",
  " "
};

const int DisplayManager::kbdRowLens[5] = {10, 10, 10, 10, 1};

DisplayManager::DisplayManager()
  : _tft(TFT_CS, TFT_DC, TFT_RST) {
}

bool DisplayManager::begin() {
  _tft.begin();
  _tft.setRotation(0);
  _tft.fillScreen(COL_BG);
  _tft.setTextSize(1);
  return true;
}

void DisplayManager::clear() {
  _tft.fillScreen(COL_BG);
  _lineCount = 0;
}

void DisplayManager::redrawTextArea() {
  int maxY = _kbdVisible ? INPUT_Y : (MAX_LINES * LINE_HEIGHT + 4);
  _tft.fillRect(0, 0, 240, maxY, COL_BG);

  int visible = _kbdVisible ? 13 : MAX_LINES;
  for (int i = 0; i < _lineCount && i < visible; i++) {
    _tft.setCursor(2, i * LINE_HEIGHT + 2);
    _tft.setTextColor(_lineColors[i]);
    _tft.setTextSize(1);
    _tft.print(_lines[i]);
  }
}

void DisplayManager::print(const String& text, uint16_t color) {
  String t = text;
  if ((int)t.length() > MAX_LINE_LEN) {
    t = t.substring(0, MAX_LINE_LEN);
  }

  int visible = _kbdVisible ? 13 : MAX_LINES;

  if (_lineCount >= visible) {
    for (int i = 1; i < visible; i++) {
      _lines[i - 1] = _lines[i];
      _lineColors[i - 1] = _lineColors[i];
    }
    _lineCount = visible - 1;
    redrawTextArea();
  }

  _lines[_lineCount] = t;
  _lineColors[_lineCount] = color;

  int y = _lineCount * LINE_HEIGHT + 2;
  _tft.fillRect(0, y, 240, LINE_HEIGHT, COL_BG);
  _tft.setCursor(2, y);
  _tft.setTextColor(color);
  _tft.setTextSize(1);
  _tft.print(t);

  _lineCount++;
}

void DisplayManager::prompt(const String& cwd) {
  String p = "CORE:";
  if (cwd == "/") {
    p += "\\>";
  } else {
    String nice = cwd;
    nice.replace("/", "\\");
    p += nice + "\\>";
  }
  print(p, COL_PROMPT);
}

void DisplayManager::drawInputLine(const String& inputBuffer) {
  int y = _kbdVisible ? INPUT_Y : (MAX_LINES * LINE_HEIGHT + 2);
  _tft.fillRect(0, y, 240, LINE_HEIGHT, COL_BG);
  _tft.setCursor(2, y);
  _tft.setTextColor(COL_PROMPT);
  _tft.setTextSize(1);

  String preview = "> " + inputBuffer + "_";
  if ((int)preview.length() > MAX_LINE_LEN) {
    preview = preview.substring(preview.length() - MAX_LINE_LEN);
  }
  _tft.print(preview);
}

void DisplayManager::drawKeyboard() {
  int startY = 190;
  _tft.fillRect(0, startY, 240, 130, COL_KBD);

  for (int r = 0; r < 5; r++) {
    int keyW = (r == 4) ? 120 : 22;
    int startX = (r == 4) ? 60 : 4;
    int y = startY + 4 + r * 24;

    for (int c = 0; c < kbdRowLens[r]; c++) {
      int x = startX + c * (keyW + 2);
      _tft.fillRoundRect(x, y, keyW, 20, 3, COL_KBD_KEY);
      _tft.setTextColor(COL_KBD_TEXT);
      _tft.setTextSize(1);
      _tft.setCursor(x + 6, y + 6);
      if (r == 4) {
        _tft.print("SPACE");
      } else {
        _tft.print(kbdRows[r][c]);
      }
    }
  }

  // DEL button
  _tft.fillRoundRect(180, startY + 100, 50, 22, 3, ILI9341_RED);
  _tft.setTextColor(ILI9341_WHITE);
  _tft.setCursor(190, startY + 106);
  _tft.print("DEL");

  // ENT button
  _tft.fillRoundRect(4, startY + 100, 50, 22, 3, ILI9341_GREEN);
  _tft.setCursor(12, startY + 106);
  _tft.print("ENT");
}

void DisplayManager::redrawAll(const String& inputBuffer) {
  _tft.fillScreen(COL_BG);
  redrawTextArea();
  drawInputLine(inputBuffer);
  if (_kbdVisible) {
    drawKeyboard();
  }
}

void DisplayManager::toggleKeyboard(const String& inputBuffer, const String& cwd) {
  _kbdVisible = !_kbdVisible;
  redrawAll(inputBuffer);
  if (!_kbdVisible) {
    prompt(cwd);
    drawInputLine(inputBuffer);
  }
}

// ============================================================
// FILE: drivers/Touch.h
// ============================================================


#include "Display.h"

enum class TouchAction {
  NONE,
  CHAR_TYPED,
  DEL_TYPED,
  COMMAND_SUBMITTED,
  KBD_TOGGLED,
  RAW_TAP
};

class TouchManager {
public:
  TouchManager();

  bool begin();
  bool isTouched();
  bool getCoordinates(int& x, int& y);
  bool getRawPoint(TS_Point& p);

  // Direct SPI probe for hardware diagnostics
  bool probeSPI(uint16_t& z1, uint16_t& z2, uint16_t& rawX, uint16_t& rawY);

  TouchAction update(DisplayManager& display, String& inputBuffer, String& submittedCmd, const String& cwd);

  void setCalibration(int minX, int maxX, int minY, int maxY);
  void setRotation(uint8_t rot);
  uint8_t getRotation() const { return _rotation; }

  int getCalMinX() const { return _calMinX; }
  int getCalMaxX() const { return _calMaxX; }
  int getCalMinY() const { return _calMinY; }
  int getCalMaxY() const { return _calMaxY; }

private:
  SPIClass _touchSPI;
  XPT2046_Touchscreen _ts;
  unsigned long _lastTouchTime = 0;
  uint8_t _rotation = 0;

  int _calMinX = 200;
  int _calMaxX = 3800;
  int _calMinY = 200;
  int _calMaxY = 3800;
};


// ============================================================
// FILE: drivers/Touch.cpp
// ============================================================

#include "Touch.h"

TouchManager::TouchManager()
  : _touchSPI(HSPI), _ts(TOUCH_CS) {
}

bool TouchManager::begin() {
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);

  pinMode(TOUCH_IRQ, INPUT_PULLUP);

  _touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  _ts.begin(_touchSPI);
  _ts.setRotation(_rotation);
  return true;
}

void TouchManager::setRotation(uint8_t rot) {
  _rotation = rot % 4;
  _ts.setRotation(_rotation);
}

bool TouchManager::probeSPI(uint16_t& z1, uint16_t& z2, uint16_t& rawX, uint16_t& rawY) {
  _touchSPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  digitalWrite(TOUCH_CS, LOW);

  _touchSPI.transfer(0xB1);
  z1 = _touchSPI.transfer16(0xC1) >> 3;
  z2 = _touchSPI.transfer16(0x91) >> 3;
  rawX = _touchSPI.transfer16(0xD1) >> 3;
  rawY = _touchSPI.transfer16(0xD0) >> 3;
  _touchSPI.transfer16(0);

  digitalWrite(TOUCH_CS, HIGH);
  _touchSPI.endTransaction();
  return true;
}

bool TouchManager::getRawPoint(TS_Point& p) {
  uint16_t z1 = 0, z2 = 0, rx = 0, ry = 0;
  probeSPI(z1, z2, rx, ry);

  if (z1 < 25 || z2 < 8) {
    return false;
  }

  Serial.printf("[DEBUG HIT] z1=%u z2=%u irq=%d\n", z1, z2, digitalRead(TOUCH_IRQ));

  p = _ts.getPoint();
  p.z = (int16_t)z1;
  return true;
}

bool TouchManager::isTouched() {
  TS_Point p;
  return getRawPoint(p);
}

bool TouchManager::getCoordinates(int& x, int& y) {
  TS_Point p;
  if (!getRawPoint(p)) return false;

  // Inverted X axis: right side is high rawX, left is low rawX
  x = map(p.x, _calMaxX, _calMinX, 0, 240);
  y = map(p.y, _calMinY, _calMaxY, 0, 320);

  if (x < 0) x = 0;
  if (x > 239) x = 239;
  if (y < 0) y = 0;
  if (y > 319) y = 319;
  return true;
}

void TouchManager::setCalibration(int minX, int maxX, int minY, int maxY) {
  _calMinX = minX;
  _calMaxX = maxX;
  _calMinY = minY;
  _calMaxY = maxY;
}

TouchAction TouchManager::update(DisplayManager& display, String& inputBuffer, String& submittedCmd, const String& cwd) {
  if (millis() - _lastTouchTime < 180) return TouchAction::NONE;

  int x = 0, y = 0;
  TS_Point p;
  if (!getRawPoint(p)) return TouchAction::NONE;
  _lastTouchTime = millis();

  x = map(p.x, _calMaxX, _calMinX, 0, 240);
  y = map(p.y, _calMinY, _calMaxY, 0, 320);
  if (x < 0) x = 0;
  if (x > 239) x = 239;
  if (y < 0) y = 0;
  if (y > 319) y = 319;

  Serial.printf("[Touch] Tap at X=%d, Y=%d (rawX=%d, rawY=%d, z=%d)\n", x, y, p.x, p.y, p.z);

  // If keyboard is hidden, tap anywhere in bottom area to open it
  if (!display.isKeyboardVisible()) {
    if (y > 180) {
      display.toggleKeyboard(inputBuffer, cwd);
      return TouchAction::KBD_TOGGLED;
    }
    return TouchAction::RAW_TAP;
  }

  int startY = 190;

  // DEL button (x > 170 && y > startY + 95)
  if (x > 170 && y > startY + 95) {
    if (inputBuffer.length() > 0) {
      inputBuffer.remove(inputBuffer.length() - 1);
      display.drawInputLine(inputBuffer);
    }
    return TouchAction::DEL_TYPED;
  }

  // ENTER button (x < 65 && y > startY + 95)
  if (x < 65 && y > startY + 95) {
    display.print("CORE:\\> " + inputBuffer, COL_PROMPT);
    submittedCmd = inputBuffer;
    inputBuffer = "";
    display.drawInputLine(inputBuffer);
    return TouchAction::COMMAND_SUBMITTED;
  }

  // Key rows
  for (int r = 0; r < 5; r++) {
    int keyW = (r == 4) ? 120 : 22;
    int startX = (r == 4) ? 60 : 4;
    int ky = startY + 4 + r * 24;

    if (y >= ky - 3 && y <= ky + 23) {
      for (int c = 0; c < DisplayManager::kbdRowLens[r]; c++) {
        int kx = startX + c * (keyW + 2);
        if (x >= kx - 3 && x <= kx + keyW + 3) {
          if (r == 4) {
            inputBuffer += ' ';
          } else {
            inputBuffer += DisplayManager::kbdRows[r][c];
          }
          display.drawInputLine(inputBuffer);
          return TouchAction::CHAR_TYPED;
        }
      }
    }
  }

  return TouchAction::NONE;
}

// ============================================================
// FILE: services/Storage.h
// ============================================================



class StorageManager {
public:
  StorageManager();

  bool begin();
  void ensureFolders();

  String normalizePath(String path);
  String resolvePath(const String& inputPath, const String& cwd);

  bool isDirectory(const String& path);
  bool exists(const String& path);

  void listDirectory(const String& path, DisplayManager& display);
  bool readFile(const String& path, DisplayManager& display);
  bool writeFile(const String& path, const String& content);
  bool removeFile(const String& path);
  bool makeDirectory(const String& path);

  size_t totalBytes() { return LittleFS.totalBytes(); }
  size_t usedBytes() { return LittleFS.usedBytes(); }
};


// ============================================================
// FILE: services/Storage.cpp
// ============================================================

#include "Storage.h"

StorageManager::StorageManager() {
}

bool StorageManager::begin() {
  if (!LittleFS.begin(true)) {
    return false;
  }
  ensureFolders();
  return true;
}

void StorageManager::ensureFolders() {
  if (!LittleFS.exists("/system")) LittleFS.mkdir("/system");
  if (!LittleFS.exists("/config")) LittleFS.mkdir("/config");
  if (!LittleFS.exists("/logs"))   LittleFS.mkdir("/logs");
}

String StorageManager::normalizePath(String path) {
  if (path.length() == 0) return "/";
  if (!path.startsWith("/")) path = "/" + path;
  while (path.indexOf("//") >= 0) path.replace("//", "/");
  if (path.length() > 1 && path.endsWith("/")) path.remove(path.length() - 1);
  return path.length() == 0 ? "/" : path;
}

String StorageManager::resolvePath(const String& inputPath, const String& cwd) {
  String full = inputPath.startsWith("/") ? inputPath : (cwd + "/" + inputPath);
  String segments[16];
  int segCount = 0;

  int start = 0;
  while (start < (int)full.length()) {
    int slash = full.indexOf('/', start);
    if (slash == -1) slash = full.length();
    String token = full.substring(start, slash);
    start = slash + 1;

    if (token.length() == 0 || token == ".") continue;
    if (token == "..") {
      if (segCount > 0) segCount--;
    } else if (segCount < 16) {
      segments[segCount++] = token;
    }
  }

  if (segCount == 0) return "/";
  String result = "";
  for (int i = 0; i < segCount; i++) {
    result += "/" + segments[i];
  }
  return result;
}

bool StorageManager::isDirectory(const String& path) {
  File f = LittleFS.open(path.c_str());
  if (!f) return false;
  bool isDir = f.isDirectory();
  f.close();
  return isDir;
}

bool StorageManager::exists(const String& path) {
  return LittleFS.exists(path.c_str());
}

void StorageManager::listDirectory(const String& path, DisplayManager& display) {
  File root = LittleFS.open(path.c_str());
  if (!root || !root.isDirectory()) {
    display.print("Not a directory", COL_ERROR);
    return;
  }

  File f = root.openNextFile();
  if (!f) {
    display.print("(empty)", COL_TEXT);
    root.close();
    return;
  }

  while (f) {
    String name = f.name();
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    if (f.isDirectory()) {
      display.print("[DIR] " + name, COL_DIR);
    } else {
      display.print(name + " " + String(f.size()) + "b", COL_TEXT);
    }
    f = root.openNextFile();
  }
  root.close();
}

bool StorageManager::readFile(const String& path, DisplayManager& display) {
  File f = LittleFS.open(path.c_str(), "r");
  if (!f || f.isDirectory()) {
    return false;
  }
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      display.print(line, COL_TEXT);
    }
  }
  f.close();
  return true;
}

bool StorageManager::writeFile(const String& path, const String& content) {
  File f = LittleFS.open(path.c_str(), "w");
  if (!f) return false;
  f.println(content);
  f.close();
  return true;
}

bool StorageManager::removeFile(const String& path) {
  return LittleFS.remove(path.c_str());
}

bool StorageManager::makeDirectory(const String& path) {
  return LittleFS.mkdir(path.c_str());
}

// ============================================================
// FILE: services/NetManager.h
// ============================================================



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


// ============================================================
// FILE: services/NetManager.cpp
// ============================================================

#include "NetManager.h"

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

// ============================================================
// FILE: services/WirelessTools.h
// ============================================================



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


// ============================================================
// FILE: services/WirelessTools.cpp
// ============================================================

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

// ============================================================
// FILE: services/Shell.h
// ============================================================


#include "Storage.h"
#include "NetManager.h"
#include "WirelessTools.h"

typedef void (*CmdFunc)(const String& args);

struct CommandEntry {
  const char* name;
  CmdFunc handler;
  const char* desc;
};

class Shell {
public:
  Shell(DisplayManager& display, TouchManager& touch, StorageManager& storage, EspNetManager& network);

  void begin();
  void update();
  void execute(String line);

  String getCwd() const { return _cwd; }
  void setCwd(const String& path) { _cwd = path; }

  DisplayManager& display() { return _display; }
  TouchManager& touch() { return _touch; }
  StorageManager& storage() { return _storage; }
  EspNetManager& network() { return _network; }

  void printPrompt();

private:
  DisplayManager& _display;
  TouchManager& _touch;
  StorageManager& _storage;
  EspNetManager& _network;

  String _cwd = "/";
  String _inputBuffer = "";

  void handleSerialInput();
  void handleTouchInput();
};

extern Shell* g_shell;


// ============================================================
// FILE: services/Shell.cpp
// ============================================================

#include "Shell.h"

Shell* g_shell = nullptr;

// ====================== Command Handlers ======================

static void cmd_help(const String& args) {
  g_shell->display().print("System:  help, clear, info, echo, mode, kbd, reboot", COL_TEXT);
  g_shell->display().print("Storage: ls, cd, pwd, cat, write, rm, mkdir", COL_TEXT);
  g_shell->display().print("Network: wifi, update", COL_TEXT);
  g_shell->display().print("Hacker:  scan, beacon, pktmon", COL_PROMPT);
  g_shell->display().print("Touch:   touch, touchtest", COL_OK);
}

static void cmd_clear(const String& args) {
  g_shell->display().clear();
  if (g_shell->display().isKeyboardVisible()) {
    g_shell->display().drawKeyboard();
  }
}

static void cmd_info(const String& args) {
  g_shell->display().print(String(OS_NAME) + " " + OS_VERSION, COL_PROMPT);
  g_shell->display().print("Heap: " + String(ESP.getFreeHeap()) + " bytes", COL_TEXT);
  g_shell->display().print("FS:   " + String(g_shell->storage().usedBytes()/1024) + "/" +
                           String(g_shell->storage().totalBytes()/1024) + " KB", COL_TEXT);
  if (g_shell->network().isConnected()) {
    g_shell->display().print("WiFi: " + g_shell->network().getSSID() + " (" + g_shell->network().getIP() + ")", COL_OK);
  } else {
    g_shell->display().print("WiFi: offline", COL_DIR);
  }
}

static void cmd_echo(const String& args) {
  if (args.length() > 0) {
    g_shell->display().print(args, COL_TEXT);
  }
}

static void cmd_mode(const String& args) {
  if (args == "status") {
    g_shell->display().setKeyboardVisible(false);
    g_shell->display().clear();
    Adafruit_ILI9341& tft = g_shell->display().raw();
    tft.setTextColor(COL_PROMPT);
    tft.setTextSize(2);
    tft.setCursor(10, 30);
    tft.println(OS_NAME);
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT);
    tft.setCursor(10, 70);
    tft.println("v" + String(OS_VERSION));
    tft.setCursor(10, 100);
    tft.println("Free Heap: " + String(ESP.getFreeHeap()));
    if (g_shell->network().isConnected()) {
      tft.setCursor(10, 130);
      tft.setTextColor(COL_OK);
      tft.println(g_shell->network().getIP());
    }
  } else {
    g_shell->display().clear();
  }
}

static void cmd_kbd(const String& args) {
  g_shell->display().toggleKeyboard("", g_shell->getCwd());
}

static void cmd_pwd(const String& args) {
  g_shell->display().print(g_shell->getCwd(), COL_TEXT);
}

static void cmd_cd(const String& args) {
  String target = (args.length() == 0 || args == "~") ? "/" : g_shell->storage().resolvePath(args, g_shell->getCwd());
  if (!g_shell->storage().isDirectory(target)) {
    g_shell->display().print("No such directory", COL_ERROR);
    return;
  }
  g_shell->setCwd(target);
}

static void cmd_ls(const String& args) {
  String path = (args.length() == 0) ? g_shell->getCwd() : g_shell->storage().resolvePath(args, g_shell->getCwd());
  g_shell->storage().listDirectory(path, g_shell->display());
}

static void cmd_cat(const String& args) {
  if (args.length() == 0) {
    g_shell->display().print("Usage: cat <file>", COL_ERROR);
    return;
  }
  String path = g_shell->storage().resolvePath(args, g_shell->getCwd());
  if (!g_shell->storage().readFile(path, g_shell->display())) {
    g_shell->display().print("File not found", COL_ERROR);
  }
}

static void cmd_write(const String& args) {
  int sp = args.indexOf(' ');
  if (sp <= 0) {
    g_shell->display().print("Usage: write <file> <text>", COL_ERROR);
    return;
  }
  String filename = args.substring(0, sp);
  String text = args.substring(sp + 1);
  String path = g_shell->storage().resolvePath(filename, g_shell->getCwd());

  if (g_shell->storage().writeFile(path, text)) {
    g_shell->display().print("OK", COL_OK);
  } else {
    g_shell->display().print("Cannot write file", COL_ERROR);
  }
}

static void cmd_rm(const String& args) {
  if (args.length() == 0) {
    g_shell->display().print("Usage: rm <file>", COL_ERROR);
    return;
  }
  String path = g_shell->storage().resolvePath(args, g_shell->getCwd());
  if (g_shell->storage().removeFile(path)) {
    g_shell->display().print("Deleted", COL_OK);
  } else {
    g_shell->display().print("Failed to delete", COL_ERROR);
  }
}

static void cmd_mkdir(const String& args) {
  if (args.length() == 0) {
    g_shell->display().print("Usage: mkdir <dir>", COL_ERROR);
    return;
  }
  String path = g_shell->storage().resolvePath(args, g_shell->getCwd());
  if (g_shell->storage().makeDirectory(path)) {
    g_shell->display().print("Created", COL_OK);
  } else {
    g_shell->display().print("Failed to create", COL_ERROR);
  }
}

static void cmd_wifi(const String& args) {
  if (args.length() == 0) {
    if (g_shell->network().isConnected()) {
      g_shell->display().print("SSID: " + g_shell->network().getSSID(), COL_TEXT);
      g_shell->display().print("IP:   " + g_shell->network().getIP(), COL_OK);
    } else {
      g_shell->display().print("WiFi: disconnected", COL_WARN);
      g_shell->display().print("Usage: wifi <ssid> <pass>", COL_TEXT);
    }
    return;
  }

  int sp = args.indexOf(' ');
  if (sp <= 0) {
    g_shell->display().print("Usage: wifi <ssid> <pass>", COL_ERROR);
    return;
  }

  String ssid = args.substring(0, sp);
  String pass = args.substring(sp + 1);
  ssid.trim();
  pass.trim();

  g_shell->network().connect(ssid, pass, g_shell->display());
}

static void cmd_update(const String& args) {
  g_shell->network().performOTA(args, g_shell->display());
}

static void cmd_reboot(const String& args) {
  g_shell->display().print("Rebooting ESP32...", COL_WARN);
  delay(500);
  ESP.restart();
}

static void cmd_scan(const String& args) {
  WirelessTools::scan(g_shell->display());
}

static void cmd_beacon(const String& args) {
  WirelessTools::beaconSpam(args, 0, g_shell->display(), g_shell->touch());
}

static void cmd_pktmon(const String& args) {
  int ch = 1;
  if (args.length() > 0) {
    ch = args.toInt();
    if (ch < 1 || ch > 14) ch = 1;
  }
  WirelessTools::packetMonitor(ch, g_shell->display(), g_shell->touch());
}

static void cmd_touchtest(const String& args) {
  g_shell->display().setKeyboardVisible(false);
  Adafruit_ILI9341& tft = g_shell->display().raw();
  tft.fillScreen(COL_BG);
  tft.setTextColor(COL_PROMPT);
  tft.setTextSize(1);
  tft.setCursor(6, 6);
  tft.print("TOUCH TEST & CALIBRATION");
  tft.setCursor(6, 20);
  tft.setTextColor(COL_WARN);
  tft.print("Draw on screen. Tap [EXIT] to exit");

  tft.fillRoundRect(175, 4, 60, 24, 4, COL_ERROR);
  tft.setTextColor(COL_TEXT);
  tft.setCursor(190, 12);
  tft.print("EXIT");

  Serial.println("=== TOUCH TEST STARTED ===");
  Serial.println("Touch anywhere on screen to see coordinates. Send any character to exit.");

  while (true) {
    if (Serial.available()) {
      while (Serial.available()) Serial.read();
      break;
    }

    TS_Point raw;
    int x = 0, y = 0;
    if (g_shell->touch().getRawPoint(raw)) {
      g_shell->touch().getCoordinates(x, y);

      // Check EXIT button
      if (x >= 170 && y <= 35) {
        break;
      }

      tft.fillRect(6, 32, 228, 16, COL_BG);
      tft.setCursor(6, 34);
      tft.setTextColor(COL_OK);
      tft.print("X:" + String(x) + " Y:" + String(y) + " (" + String(raw.x) + "," + String(raw.y) + ",z=" + String(raw.z) + ")");
      tft.fillCircle(x, y, 2, COL_PROMPT);

      Serial.printf("[TOUCH] X=%d Y=%d (raw: %d, %d, z=%d)\n", x, y, raw.x, raw.y, raw.z);
      delay(30);
    }
    delay(10);
  }

  Serial.println("=== TOUCH TEST ENDED ===");
  g_shell->display().clear();
}

static void cmd_touch(const String& args) {
  int sp = args.indexOf(' ');
  String sub = (sp < 0) ? args : args.substring(0, sp);
  String subArgs = (sp < 0) ? "" : args.substring(sp + 1);
  sub.toLowerCase();

  if (sub == "rot") {
    int r = subArgs.toInt();
    g_shell->touch().setRotation(r);
    g_shell->display().print("Touch rotation set to " + String(r % 4), COL_OK);
    Serial.println("[Touch] Rotation set to " + String(r % 4));
    return;
  }

  if (sub == "info") {
    g_shell->display().print("Touch Rot: " + String(g_shell->touch().getRotation()), COL_TEXT);
    g_shell->display().print("Cal X: " + String(g_shell->touch().getCalMinX()) + ".." + String(g_shell->touch().getCalMaxX()), COL_TEXT);
    g_shell->display().print("Cal Y: " + String(g_shell->touch().getCalMinY()) + ".." + String(g_shell->touch().getCalMaxY()), COL_TEXT);
    return;
  }

  if (sub == "cal") {
    int p1 = subArgs.indexOf(' ');
    if (p1 > 0) {
      int p2 = subArgs.indexOf(' ', p1 + 1);
      int p3 = (p2 > 0) ? subArgs.indexOf(' ', p2 + 1) : -1;
      if (p1 > 0 && p2 > 0 && p3 > 0) {
        int x1 = subArgs.substring(0, p1).toInt();
        int x2 = subArgs.substring(p1 + 1, p2).toInt();
        int y1 = subArgs.substring(p2 + 1, p3).toInt();
        int y2 = subArgs.substring(p3 + 1).toInt();
        g_shell->touch().setCalibration(x1, x2, y1, y2);
        g_shell->display().print("Calibrated: X(" + String(x1) + ".." + String(x2) + ") Y(" + String(y1) + ".." + String(y2) + ")", COL_OK);
        return;
      }
    }
    g_shell->display().print("Usage: touch cal <minX> <maxX> <minY> <maxY>", COL_ERROR);
    return;
  }

  if (sub == "irq") {
    Serial.println("Reading TOUCH_IRQ pin 27 (press any key to stop):");
    while (!Serial.available()) {
      Serial.printf("IRQ pin %d: %d\n", TOUCH_IRQ, digitalRead(TOUCH_IRQ));
      delay(150);
    }
    while (Serial.available()) Serial.read();
    return;
  }

  if (sub == "raw") {
    g_shell->display().print("Streaming raw SPI to Serial...", COL_WARN);
    g_shell->display().print("Send character in Serial to stop", COL_TEXT);
    Serial.println("=== XPT2046 RAW SPI DIAGNOSTIC STREAM ===");
    Serial.println("Format: Z1, Z2, Pressure(Z), rawX, rawY | lib: x, y, z");
    while (!Serial.available()) {
      uint16_t z1 = 0, z2 = 0, rx = 0, ry = 0;
      g_shell->touch().probeSPI(z1, z2, rx, ry);
      int pz = (int)z1 + 4095 - (int)z2;
      TS_Point p;
      bool ok = g_shell->touch().getRawPoint(p);

      if (pz > 150 || ok || rx > 50 || ry > 50) {
        Serial.printf("[SPI HIT] Z1=%u Z2=%u P=%d rx=%u ry=%u | lib: x=%d y=%d z=%d\n",
                      z1, z2, pz, rx, ry, p.x, p.y, p.z);
      }
      delay(40);
    }
    while (Serial.available()) Serial.read();
    g_shell->display().print("Stopped", COL_OK);
    Serial.println("=== DIAGNOSTIC STREAM STOPPED ===");
    return;
  }

  if (sub == "test" || sub.length() == 0) {
    cmd_touchtest("");
    return;
  }

  g_shell->display().print("Usage: touch [test|raw|rot <0..3>|cal|info]", COL_DIR);
}

// ====================== Command Table ======================

static const CommandEntry cmdTable[] = {
  {"help",      cmd_help,      "List available commands"},
  {"clear",     cmd_clear,     "Clear the screen"},
  {"info",      cmd_info,      "System information"},
  {"echo",      cmd_echo,      "Print text"},
  {"mode",      cmd_mode,      "Switch display mode (shell/status)"},
  {"kbd",       cmd_kbd,       "Toggle on-screen keyboard"},
  {"pwd",       cmd_pwd,       "Print working directory"},
  {"cd",        cmd_cd,        "Change directory"},
  {"ls",        cmd_ls,        "List directory contents"},
  {"dir",       cmd_ls,        "Alias for ls"},
  {"cat",       cmd_cat,       "View file contents"},
  {"write",     cmd_write,     "Write text to file"},
  {"rm",        cmd_rm,        "Remove file"},
  {"mkdir",     cmd_mkdir,     "Create directory"},
  {"wifi",      cmd_wifi,      "WiFi status / connect"},
  {"update",    cmd_update,    "OTA firmware update from URL"},
  {"reboot",    cmd_reboot,    "Restart ESP32"},
  {"scan",      cmd_scan,      "Scan 2.4GHz WiFi networks"},
  {"beacon",    cmd_beacon,    "Beacon spam (fake APs)"},
  {"pktmon",    cmd_pktmon,    "Packet traffic monitor (promiscuous mode)"},
  {"touch",     cmd_touch,     "Touchscreen diagnostics & calibration"},
  {"touchtest", cmd_touchtest, "Interactive touch drawing & coordinate test"}
};

static const int cmdTableSize = sizeof(cmdTable) / sizeof(cmdTable[0]);

// ====================== Shell Class ======================

Shell::Shell(DisplayManager& display, TouchManager& touch, StorageManager& storage, EspNetManager& network)
  : _display(display), _touch(touch), _storage(storage), _network(network) {
  g_shell = this;
}

void Shell::begin() {
  _display.prompt(_cwd);
  _display.drawInputLine(_inputBuffer);
}

void Shell::printPrompt() {
  _display.prompt(_cwd);
  _display.drawInputLine(_inputBuffer);
}

void Shell::execute(String line) {
  line.trim();
  if (line.length() == 0) return;

  int sp = line.indexOf(' ');
  String cmd = (sp < 0) ? line : line.substring(0, sp);
  String args = (sp < 0) ? "" : line.substring(sp + 1);
  args.trim();
  cmd.toLowerCase();

  for (int i = 0; i < cmdTableSize; i++) {
    if (cmd == cmdTable[i].name) {
      cmdTable[i].handler(args);
      return;
    }
  }

  _display.print("Unknown command: " + cmd, COL_ERROR);
}

void Shell::update() {
  handleTouchInput();
  handleSerialInput();
}

void Shell::handleTouchInput() {
  String submitted;
  TouchAction action = _touch.update(_display, _inputBuffer, submitted, _cwd);

  if (action == TouchAction::COMMAND_SUBMITTED) {
    execute(submitted);
    printPrompt();
  }
}

void Shell::handleSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;

    if (c == '\n') {
      _display.print("CORE:\\> " + _inputBuffer, COL_PROMPT);
      String cmd = _inputBuffer;
      _inputBuffer = "";
      _display.drawInputLine(_inputBuffer);
      execute(cmd);
      printPrompt();
    }
    else if (c == 8 || c == 127) { // Backspace
      if (_inputBuffer.length() > 0) {
        _inputBuffer.remove(_inputBuffer.length() - 1);
        _display.drawInputLine(_inputBuffer);
      }
    }
    else if (_inputBuffer.length() < 60) {
      _inputBuffer += c;
      _display.drawInputLine(_inputBuffer);
    }
  }
}

// ============================================================
// FILE: kernel/Kernel.h
// ============================================================



class Kernel {
public:
  Kernel();

  bool begin();
  void loop();

  DisplayManager& display() { return _display; }
  TouchManager& touch() { return _touch; }
  StorageManager& storage() { return _storage; }
  EspNetManager& network() { return _network; }
  Shell& shell() { return _shell; }

private:
  DisplayManager _display;
  TouchManager _touch;
  StorageManager _storage;
  EspNetManager _network;
  Shell _shell;
};

extern Kernel* g_kernel;


// ============================================================
// FILE: kernel/Kernel.cpp
// ============================================================

#include "Kernel.h"

Kernel* g_kernel = nullptr;

Kernel::Kernel()
  : _shell(_display, _touch, _storage, _network) {
  g_kernel = this;
}

bool Kernel::begin() {
  Serial.begin(SERIAL_BAUD);
  delay(300);

  _display.begin();
  _touch.begin();

  if (!_storage.begin()) {
    _display.print("LittleFS failed!", COL_ERROR);
  }

  _display.print(String(OS_NAME) + " " + OS_VERSION, COL_PROMPT);
  _display.print("Marauder tools ready", COL_OK);

  _network.tryAutoConnect(_display);
  _shell.begin();

  Serial.println("[Kernel] " OS_NAME " " OS_VERSION " started");
  return true;
}

void Kernel::loop() {
  _shell.update();
}

// ============================================================
// FILE: EspOS.ino
// ============================================================

/*
 * EspOS - Minimal modular OS for ESP32
 * v0.5.0-marauder: Modular Architecture + Marauder Wireless Tools
 */


Kernel kernel;

void setup() {
  kernel.begin();
}

void loop() {
  kernel.loop();
}
