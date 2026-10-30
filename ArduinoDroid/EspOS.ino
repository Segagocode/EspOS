/*
 * EspOS v0.4.0-alpha
 * Major update:
 * - Visible input line
 * - Keyboard with . / -
 * - Fixed scrolling
 * - Better resolvePath
 * - Command table
 * - WiFi + OTA update
 */

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

// ====================== PINS ======================
#define TFT_CS     15
#define TFT_DC     2
#define TFT_RST    4

#define TOUCH_CS   33
#define TOUCH_IRQ  34

// Shared SPI expected:
// SCK  = 18
// MOSI = 23
// MISO = 19

// ====================== CONFIG ======================
#define MAX_LINES        14
#define LINE_HEIGHT      12
#define MAX_LINE_LEN     38
#define INPUT_Y          172

#define SERIAL_BAUD      115200

#define OS_NAME          "EspOS"
#define OS_VERSION       "0.4.0-alpha"

#define COL_BG           ILI9341_BLACK
#define COL_TEXT         ILI9341_WHITE
#define COL_PROMPT       ILI9341_CYAN
#define COL_ERROR        ILI9341_RED
#define COL_OK           ILI9341_GREEN
#define COL_DIR          ILI9341_YELLOW
#define COL_KBD          ILI9341_DARKGREY
#define COL_KBD_KEY      ILI9341_LIGHTGREY
#define COL_KBD_TEXT     ILI9341_BLACK
#define COL_BAR_BG       ILI9341_DARKGREY

// ====================== OBJECTS ======================
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

String lines[MAX_LINES];
uint16_t lineColors[MAX_LINES];
int lineCount = 0;

String cwd = "/";
String inputBuffer = "";
bool kbdVisible = false;
bool wifiConnecting = false;

// Keyboard layout with path-friendly symbols
const char* kbdRows[] = {
  "1234567890",
  "qwertyuiop",
  "asdfghjkl/",
  "zxcvbnm._-",
  " "
};
int kbdRowLens[] = {10, 10, 10, 10, 1};

// ====================== FORWARD ======================
void processCommand(String line);
void displayPrint(const String& text, uint16_t color);
void displayPrompt();
void drawInputLine();
void redrawAll();
void drawKeyboard();

// ====================== PATH ======================

String normalizePath(String path) {
  if (path.length() == 0) return "/";
  if (!path.startsWith("/")) path = "/" + path;
  while (path.indexOf("//") >= 0) path.replace("//", "/");
  if (path.length() > 1 && path.endsWith("/")) path.remove(path.length() - 1);
  return path.length() == 0 ? "/" : path;
}

String resolvePath(const String& inputPath) {
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
  for (int i = 0; i < segCount; i++) result += "/" + segments[i];
  return result;
}

void ensureFolders() {
  if (!LittleFS.exists("/system")) LittleFS.mkdir("/system");
  if (!LittleFS.exists("/config"))  LittleFS.mkdir("/config");
  if (!LittleFS.exists("/logs"))    LittleFS.mkdir("/logs");
}

// ====================== DISPLAY ======================

void displayClear() {
  tft.fillScreen(COL_BG);
  lineCount = 0;
}

void redrawTextArea() {
  int maxY = kbdVisible ? INPUT_Y : (MAX_LINES * LINE_HEIGHT + 4);
  tft.fillRect(0, 0, 240, maxY, COL_BG);

  int visible = kbdVisible ? 13 : MAX_LINES;
  for (int i = 0; i < lineCount && i < visible; i++) {
    tft.setCursor(2, i * LINE_HEIGHT + 2);
    tft.setTextColor(lineColors[i]);
    tft.setTextSize(1);
    tft.print(lines[i]);
  }
}

void displayPrint(const String& text, uint16_t color) {
  String t = text;
  if (t.length() > MAX_LINE_LEN) t = t.substring(0, MAX_LINE_LEN);

  int visible = kbdVisible ? 13 : MAX_LINES;

  if (lineCount >= visible) {
    for (int i = 1; i < visible; i++) {
      lines[i - 1] = lines[i];
      lineColors[i - 1] = lineColors[i];
    }
    lineCount = visible - 1;
    redrawTextArea();
  }

  lines[lineCount] = t;
  lineColors[lineCount] = color;

  int y = lineCount * LINE_HEIGHT + 2;
  tft.fillRect(0, y, 240, LINE_HEIGHT, COL_BG);
  tft.setCursor(2, y);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.print(t);

  lineCount++;
}

void displayPrompt() {
  String p = "CORE:";
  if (cwd == "/") p += "\\>";
  else {
    String nice = cwd;
    nice.replace("/", "\\");
    p += nice + "\\>";
  }
  displayPrint(p, COL_PROMPT);
  drawInputLine();
}

void drawInputLine() {
  int y = kbdVisible ? INPUT_Y : (MAX_LINES * LINE_HEIGHT + 2);
  tft.fillRect(0, y, 240, LINE_HEIGHT, COL_BG);
  tft.setCursor(2, y);
  tft.setTextColor(COL_PROMPT);
  tft.setTextSize(1);

  String preview = "> " + inputBuffer + "_";
  if (preview.length() > MAX_LINE_LEN) {
    preview = preview.substring(preview.length() - MAX_LINE_LEN);
  }
  tft.print(preview);
}

void drawKeyboard() {
  int startY = 190;
  tft.fillRect(0, startY, 240, 130, COL_KBD);

  for (int r = 0; r < 5; r++) {
    int keyW = (r == 4) ? 120 : 22;
    int startX = (r == 4) ? 60 : 4;
    int y = startY + 4 + r * 24;

    for (int c = 0; c < kbdRowLens[r]; c++) {
      int x = startX + c * (keyW + 2);
      tft.fillRoundRect(x, y, keyW, 20, 3, COL_KBD_KEY);
      tft.setTextColor(COL_KBD_TEXT);
      tft.setTextSize(1);
      tft.setCursor(x + 6, y + 6);
      if (r == 4) tft.print("SPACE");
      else tft.print(kbdRows[r][c]);
    }
  }

  tft.fillRoundRect(180, startY + 100, 50, 22, 3, ILI9341_RED);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(190, startY + 106);
  tft.print("DEL");

  tft.fillRoundRect(4, startY + 100, 50, 22, 3, ILI9341_GREEN);
  tft.setCursor(12, startY + 106);
  tft.print("ENT");
}

void redrawAll() {
  tft.fillScreen(COL_BG);
  redrawTextArea();
  drawInputLine();
  if (kbdVisible) drawKeyboard();
}

void toggleKeyboard() {
  kbdVisible = !kbdVisible;
  redrawAll();
  if (!kbdVisible) displayPrompt();
}

// ====================== TOUCH ======================

void handleTouch() {
  static unsigned long lastTouch = 0;
  if (millis() - lastTouch < 220) return;
  if (!ts.touched()) return;

  TS_Point p = ts.getPoint();
  int x = map(p.x, 200, 3800, 0, 240);
  int y = map(p.y, 200, 3800, 0, 320);
  if (x < 0) x = 0;
  if (x > 239) x = 239;
  if (y < 0) y = 0;
  if (y > 319) y = 319;

  lastTouch = millis();

  if (!kbdVisible) {
    if (y > 270) toggleKeyboard();
    return;
  }

  int startY = 190;

  // DEL
  if (x > 170 && y > startY + 95) {
    if (inputBuffer.length() > 0) inputBuffer.remove(inputBuffer.length() - 1);
    drawInputLine();
    return;
  }

  // ENTER
  if (x < 65 && y > startY + 95) {
    displayPrint("CORE:\\> " + inputBuffer, COL_PROMPT);
    String cmd = inputBuffer;
    inputBuffer = "";
    drawInputLine();
    processCommand(cmd);
    displayPrompt();
    return;
  }

  // Keys
  for (int r = 0; r < 5; r++) {
    int keyW = (r == 4) ? 120 : 22;
    int startX = (r == 4) ? 60 : 4;
    int ky = startY + 4 + r * 24;

    if (y >= ky - 3 && y <= ky + 23) {
      for (int c = 0; c < kbdRowLens[r]; c++) {
        int kx = startX + c * (keyW + 2);
        if (x >= kx - 3 && x <= kx + keyW + 3) {
          if (r == 4) inputBuffer += ' ';
          else inputBuffer += kbdRows[r][c];
          drawInputLine();
          return;
        }
      }
    }
  }
}

// ====================== WIFI / OTA ======================

bool loadWifiConfig(String& ssid, String& pass) {
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

void saveWifiConfig(const String& ssid, const String& pass) {
  File f = LittleFS.open("/config/wifi.cfg", "w");
  if (!f) return;
  f.println(ssid);
  f.println(pass);
  f.close();
}

void tryAutoWifi() {
  String ssid, pass;
  if (!loadWifiConfig(ssid, pass)) return;

  displayPrint("WiFi auto: " + ssid, COL_TEXT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    displayPrint("IP: " + WiFi.localIP().toString(), COL_OK);
  } else {
    displayPrint("WiFi auto failed", COL_ERROR);
  }
}

void cmd_wifi(const String& args) {
  if (args.length() == 0) {
    if (WiFi.status() == WL_CONNECTED) {
      displayPrint("SSID: " + WiFi.SSID(), COL_TEXT);
      displayPrint("IP: " + WiFi.localIP().toString(), COL_OK);
    } else {
      displayPrint("WiFi: disconnected", COL_ERROR);
      displayPrint("Usage: wifi <ssid> <pass>", COL_TEXT);
    }
    return;
  }

  int sp = args.indexOf(' ');
  if (sp <= 0) {
    displayPrint("Usage: wifi <ssid> <pass>", COL_ERROR);
    return;
  }

  String ssid = args.substring(0, sp);
  String pass = args.substring(sp + 1);
  ssid.trim();
  pass.trim();

  displayPrint("Connecting: " + ssid, COL_TEXT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    displayPrint("Connected", COL_OK);
    displayPrint("IP: " + WiFi.localIP().toString(), COL_OK);
    saveWifiConfig(ssid, pass);
    displayPrint("Saved to /config/wifi.cfg", COL_TEXT);
  } else {
    displayPrint("Connect failed", COL_ERROR);
  }
}

void cmd_update(const String& args) {
  if (WiFi.status() != WL_CONNECTED) {
    displayPrint("WiFi not connected", COL_ERROR);
    return;
  }

  String url = args.length() > 0 ? args : "";
  if (url.length() == 0) {
    displayPrint("Usage: update <url>", COL_ERROR);
    displayPrint("URL to .bin firmware", COL_TEXT);
    return;
  }

  kbdVisible = false;
  displayClear();
  displayPrint("OTA Update...", COL_PROMPT);
  displayPrint(url, COL_TEXT);

  tft.drawRect(18, 140, 204, 24, COL_TEXT);
  tft.fillRect(20, 142, 0, 20, COL_OK);

  WiFiClientSecure client;
  client.setInsecure();

  httpUpdate.rebootOnUpdate(true);
  httpUpdate.onProgress([](int cur, int total) {
    if (total <= 0) return;
    int pct = (cur * 100) / total;
    int bar = (pct * 200) / 100;
    tft.fillRect(20, 142, bar, 20, COL_OK);
    tft.fillRect(90, 170, 60, 14, COL_BG);
    tft.setCursor(100, 172);
    tft.setTextColor(COL_TEXT);
    tft.setTextSize(1);
    tft.print(String(pct) + "%");
  });

  t_httpUpdate_return ret = httpUpdate.update(client, url);

  if (ret == HTTP_UPDATE_FAILED) {
    displayPrint("OTA failed", COL_ERROR);
    displayPrint(httpUpdate.getLastErrorString(), COL_ERROR);
  } else if (ret == HTTP_UPDATE_NO_UPDATES) {
    displayPrint("No updates", COL_TEXT);
  }
}

// ====================== COMMANDS ======================

typedef void (*CmdHandler)(const String& args);

struct CommandEntry {
  const char* name;
  CmdHandler handler;
};

void cmd_help(const String& args);
void cmd_clear(const String& args);
void cmd_info(const String& args);
void cmd_echo(const String& args);
void cmd_mode(const String& args);
void cmd_kbd(const String& args);
void cmd_pwd(const String& args);
void cmd_cd(const String& args);
void cmd_ls(const String& args);
void cmd_cat(const String& args);
void cmd_write(const String& args);
void cmd_rm(const String& args);
void cmd_mkdir(const String& args);
void cmd_wifi(const String& args);
void cmd_update(const String& args);

const CommandEntry cmdTable[] = {
  {"help",   cmd_help},
  {"clear",  cmd_clear},
  {"info",   cmd_info},
  {"echo",   cmd_echo},
  {"mode",   cmd_mode},
  {"kbd",    cmd_kbd},
  {"pwd",    cmd_pwd},
  {"cd",     cmd_cd},
  {"ls",     cmd_ls},
  {"dir",    cmd_ls},
  {"cat",    cmd_cat},
  {"write",  cmd_write},
  {"rm",     cmd_rm},
  {"mkdir",  cmd_mkdir},
  {"wifi",   cmd_wifi},
  {"update", cmd_update}
};

const int cmdTableSize = sizeof(cmdTable) / sizeof(cmdTable[0]);

void cmd_help(const String& args) {
  displayPrint("help clear info echo mode kbd", COL_TEXT);
  displayPrint("ls cd pwd cat write rm mkdir", COL_TEXT);
  displayPrint("wifi update", COL_TEXT);
}

void cmd_clear(const String& args) {
  displayClear();
  if (kbdVisible) drawKeyboard();
  displayPrompt();
}

void cmd_info(const String& args) {
  displayPrint(String(OS_NAME) + " " + OS_VERSION, COL_PROMPT);
  displayPrint("Heap: " + String(ESP.getFreeHeap()), COL_TEXT);
  displayPrint("FS: " + String(LittleFS.usedBytes()/1024) + "/" + String(LittleFS.totalBytes()/1024) + " KB", COL_TEXT);
  if (WiFi.status() == WL_CONNECTED) {
    displayPrint("IP: " + WiFi.localIP().toString(), COL_OK);
  } else {
    displayPrint("WiFi: offline", COL_TEXT);
  }
}

void cmd_echo(const String& args) {
  if (args.length()) displayPrint(args, COL_TEXT);
}

void cmd_mode(const String& args) {
  if (args == "status") {
    kbdVisible = false;
    displayClear();
    tft.setTextColor(COL_PROMPT);
    tft.setTextSize(2);
    tft.setCursor(10, 30);
    tft.println(OS_NAME);
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT);
    tft.setCursor(10, 70);
    tft.println("v" + String(OS_VERSION));
    tft.setCursor(10, 100);
    tft.println("Heap: " + String(ESP.getFreeHeap()));
    if (WiFi.status() == WL_CONNECTED) {
      tft.setCursor(10, 130);
      tft.setTextColor(COL_OK);
      tft.println(WiFi.localIP().toString());
    }
  } else {
    displayClear();
    displayPrompt();
  }
}

void cmd_kbd(const String& args) {
  toggleKeyboard();
}

void cmd_pwd(const String& args) {
  displayPrint(cwd, COL_TEXT);
}

void cmd_cd(const String& args) {
  String target = (args.length() == 0 || args == "~") ? "/" : resolvePath(args);
  File dir = LittleFS.open(target.c_str());
  if (!dir || !dir.isDirectory()) {
    displayPrint("No such directory", COL_ERROR);
    return;
  }
  dir.close();
  cwd = target;
}

void cmd_ls(const String& args) {
  String path = args.length() == 0 ? cwd : resolvePath(args);
  File root = LittleFS.open(path.c_str());
  if (!root || !root.isDirectory()) {
    displayPrint("Not a directory", COL_ERROR);
    return;
  }

  File f = root.openNextFile();
  if (!f) {
    displayPrint("(empty)", COL_TEXT);
    root.close();
    return;
  }

  while (f) {
    String name = f.name();
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    if (f.isDirectory()) displayPrint("[DIR] " + name, COL_DIR);
    else displayPrint(name + " " + String(f.size()) + "b", COL_TEXT);
    f = root.openNextFile();
  }
  root.close();
}

void cmd_cat(const String& args) {
  if (args.length() == 0) {
    displayPrint("Usage: cat <file>", COL_ERROR);
    return;
  }
  File f = LittleFS.open(resolvePath(args).c_str(), "r");
  if (!f || f.isDirectory()) {
    displayPrint("File not found", COL_ERROR);
    return;
  }
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length()) displayPrint(line, COL_TEXT);
  }
  f.close();
}

void cmd_write(const String& args) {
  int sp = args.indexOf(' ');
  if (sp <= 0) {
    displayPrint("Usage: write <file> <text>", COL_ERROR);
    return;
  }
  File f = LittleFS.open(resolvePath(args.substring(0, sp)).c_str(), "w");
  if (!f) {
    displayPrint("Cannot write", COL_ERROR);
    return;
  }
  f.println(args.substring(sp + 1));
  f.close();
  displayPrint("OK", COL_OK);
}

void cmd_rm(const String& args) {
  if (args.length() == 0) {
    displayPrint("Usage: rm <file>", COL_ERROR);
    return;
  }
  if (LittleFS.remove(resolvePath(args).c_str())) displayPrint("Deleted", COL_OK);
  else displayPrint("Failed", COL_ERROR);
}

void cmd_mkdir(const String& args) {
  if (args.length() == 0) {
    displayPrint("Usage: mkdir <dir>", COL_ERROR);
    return;
  }
  if (LittleFS.mkdir(resolvePath(args).c_str())) displayPrint("Created", COL_OK);
  else displayPrint("Failed", COL_ERROR);
}

void processCommand(String line) {
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
  displayPrint("Unknown command", COL_ERROR);
}

// ====================== SETUP / LOOP ======================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(400);

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(COL_BG);
  tft.setTextSize(1);

  ts.begin();
  ts.setRotation(0);

  if (!LittleFS.begin(true)) {
    displayPrint("LittleFS failed", COL_ERROR);
  } else {
    ensureFolders();
  }

  displayPrint(String(OS_NAME) + " " + OS_VERSION, COL_PROMPT);
  displayPrint("WiFi + OTA ready", COL_OK);

  tryAutoWifi();
  displayPrompt();

  Serial.println("[EspOS] v0.4.0-alpha started");
}

void loop() {
  handleTouch();

  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;

    if (c == '\n') {
      displayPrint("CORE:\\> " + inputBuffer, COL_PROMPT);
      String cmd = inputBuffer;
      inputBuffer = "";
      drawInputLine();
      processCommand(cmd);
      displayPrompt();
    }
    else if (c == 8 || c == 127) {
      if (inputBuffer.length() > 0) {
        inputBuffer.remove(inputBuffer.length() - 1);
        drawInputLine();
      }
    }
    else if (inputBuffer.length() < 60) {
      inputBuffer += c;
      drawInputLine();
    }
  }
}
