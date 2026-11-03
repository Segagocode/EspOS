#include "Shell.h"
#include "../config/Config.h"

Shell* g_shell = nullptr;

// ====================== Command Handlers ======================

static void cmd_help(const String& args) {
  g_shell->display().print("System:  help, clear, info, echo, mode, kbd, reboot", COL_TEXT);
  g_shell->display().print("Storage: ls, cd, pwd, cat, write, rm, mkdir", COL_TEXT);
  g_shell->display().print("Network: wifi, update", COL_TEXT);
  g_shell->display().print("Hacker:  scan, beacon, pktmon", COL_PROMPT);
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
  tft.print("TOUCH TEST (Draw on screen)");
  tft.setCursor(6, 20);
  tft.setTextColor(COL_WARN);
  tft.print("Tap [EXIT] button to exit");

  tft.fillRoundRect(175, 4, 60, 24, 4, COL_ERROR);
  tft.setTextColor(COL_TEXT);
  tft.setCursor(190, 12);
  tft.print("EXIT");

  while (true) {
    if (Serial.available()) {
      while (Serial.available()) Serial.read();
      break;
    }
    if (g_shell->touch().isTouched()) {
      int x = 0, y = 0;
      TS_Point raw;
      if (g_shell->touch().getCoordinates(x, y) && g_shell->touch().getRawPoint(raw)) {
        if (x >= 165 && y <= 35) {
          break;
        }
        tft.fillRect(6, 32, 228, 16, COL_BG);
        tft.setCursor(6, 34);
        tft.setTextColor(COL_OK);
        tft.print("X:" + String(x) + " Y:" + String(y) + " (" + String(raw.x) + "," + String(raw.y) + ")");
        tft.fillCircle(x, y, 2, COL_PROMPT);
      }
      delay(30);
    }
    delay(10);
  }

  g_shell->display().clear();
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
