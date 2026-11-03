#include "Shell.h"
#include "../config/Config.h"

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

      if (z1 >= 15 || z2 >= 5 || ok) {
        Serial.printf("[SPI HIT] Z1=%u Z2=%u rx=%u ry=%u | lib: x=%d y=%d z=%d\n",
                      z1, z2, rx, ry, p.x, p.y, p.z);
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
