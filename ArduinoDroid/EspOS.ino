/*
 * EspOS - Single-file version for ArduinoDroid
 * v0.2.0-alpha
 *
 * Stage 2: LittleFS as system volume (CORE:)
 * + basic file commands
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FS.h>
#include <LittleFS.h>

// ====================== CONFIG ======================
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_ADDRESS        0x3C
#define OLED_RESET_PIN      -1

#define MAX_LINES           8
#define MAX_LINE_LENGTH     21

#define SERIAL_BAUD         115200

#define OS_NAME             "EspOS"
#define OS_VERSION          "0.2.0-alpha"

#define SYSTEM_PROMPT       "CORE:\\>"

// ====================== FORWARD ======================
class Kernel;
Kernel* g_kernel = nullptr;

// ====================== DISPLAY ======================

enum class DisplayMode {
  SHELL,
  STATUS
};

class DisplayManager {
public:
  DisplayManager() : _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN) {}

  bool begin() {
    if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
      return false;
    }
    _display.clearDisplay();
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.display();
    return true;
  }

  void clear() {
    _display.clearDisplay();
    _display.display();
  }

  void setMode(DisplayMode mode) {
    _mode = mode;
    redraw();
  }

  DisplayMode getMode() const {
    return _mode;
  }

  void printLine(const String& text) {
    String truncated = text;
    if (truncated.length() > MAX_LINE_LENGTH) {
      truncated = truncated.substring(0, MAX_LINE_LENGTH);
    }

    if (_lineCount < MAX_LINES) {
      _lines[_lineCount] = truncated;
      _lineCount++;
    } else {
      for (int i = 1; i < MAX_LINES; i++) {
        _lines[i - 1] = _lines[i];
      }
      _lines[MAX_LINES - 1] = truncated;
    }
    redraw();
  }

  void clearLines() {
    _lineCount = 0;
    for (int i = 0; i < MAX_LINES; i++) {
      _lines[i] = "";
    }
    redraw();
  }

  void redraw() {
    _display.clearDisplay();
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(0, 0);

    switch (_mode) {
      case DisplayMode::SHELL:
        drawShell();
        break;
      case DisplayMode::STATUS:
        drawStatus();
        break;
    }

    _display.display();
  }

private:
  Adafruit_SSD1306 _display;
  DisplayMode _mode = DisplayMode::SHELL;
  String _lines[MAX_LINES];
  int _lineCount = 0;

  void drawShell() {
    for (int i = 0; i < _lineCount; i++) {
      _display.setCursor(0, i * 8);
      _display.print(_lines[i]);
    }
  }

  void drawStatus() {
    _display.setCursor(0, 0);
    _display.println(OS_NAME);
    _display.print("v");
    _display.println(OS_VERSION);
    _display.println("------------");
    _display.println("Mode: STATUS");
    _display.println("Heap: " + String(ESP.getFreeHeap()));
    _display.println("FS: LittleFS");
  }
};

// ====================== COMMAND REGISTRY ======================

typedef void (*CommandHandler)(const String& args);

struct Command {
  const char* name;
  const char* description;
  CommandHandler handler;
};

class CommandRegistry {
public:
  static const int MAX_COMMANDS = 24;

  void registerCommand(const char* name, const char* description, CommandHandler handler) {
    if (_count >= MAX_COMMANDS) return;
    _commands[_count].name = name;
    _commands[_count].description = description;
    _commands[_count].handler = handler;
    _count++;
  }

  bool execute(const String& input) {
    String cmd = input;
    cmd.trim();
    if (cmd.length() == 0) return false;

    int spaceIndex = cmd.indexOf(' ');
    String name;
    String args = "";

    if (spaceIndex == -1) {
      name = cmd;
    } else {
      name = cmd.substring(0, spaceIndex);
      args = cmd.substring(spaceIndex + 1);
      args.trim();
    }

    int idx = findCommand(name);
    if (idx == -1) return false;

    _commands[idx].handler(args);
    return true;
  }

  void listCommands(String& output) {
    output = "";
    for (int i = 0; i < _count; i++) {
      output += String(_commands[i].name);
      output += " - ";
      output += _commands[i].description;
      if (i < _count - 1) output += "\n";
    }
  }

private:
  Command _commands[MAX_COMMANDS];
  int _count = 0;

  int findCommand(const String& name) {
    for (int i = 0; i < _count; i++) {
      if (name.equalsIgnoreCase(_commands[i].name)) return i;
    }
    return -1;
  }
};

// ====================== SHELL ======================

class Shell {
public:
  Shell(DisplayManager& display, CommandRegistry& registry)
    : _display(display), _registry(registry) {}

  void begin() {
    _display.setMode(DisplayMode::SHELL);
    _display.clearLines();
    printPrompt();
  }

  void printPrompt() {
    _display.printLine(SYSTEM_PROMPT);
  }

  void printResponse(const String& text) {
    _display.printLine(text);
  }

  void update() {
    while (Serial.available()) {
      char c = Serial.read();

      if (c == '\r') continue;

      if (c == '\n') {
        if (_currentLine.length() > 0) {
          _display.printLine(String(SYSTEM_PROMPT) + " " + _currentLine);
          processLine(_currentLine);
          _currentLine = "";
        }
        printPrompt();
      } else if (c == 8 || c == 127) {
        if (_currentLine.length() > 0) {
          _currentLine.remove(_currentLine.length() - 1);
        }
      } else {
        if (_currentLine.length() < MAX_LINE_LENGTH - 6) {
          _currentLine += c;
        }
      }
    }
  }

private:
  DisplayManager& _display;
  CommandRegistry& _registry;
  String _currentLine;

  void processLine(const String& line) {
    if (!_registry.execute(line)) {
      _display.printLine("Unknown command");
    }
  }
};

// ====================== FILE SYSTEM HELPERS ======================

bool ensureSystemFolders() {
  if (!LittleFS.exists("/system")) {
    LittleFS.mkdir("/system");
  }
  if (!LittleFS.exists("/config")) {
    LittleFS.mkdir("/config");
  }
  if (!LittleFS.exists("/logs")) {
    LittleFS.mkdir("/logs");
  }
  return true;
}

// ====================== KERNEL ======================

class Kernel {
public:
  Kernel() : _shell(_display, _commands) {
    g_kernel = this;
  }

  bool begin() {
    Serial.begin(SERIAL_BAUD);
    delay(100);

    // --- Display ---
    if (!_display.begin()) {
      Serial.println("[Kernel] Display init failed!");
      return false;
    }

    // --- LittleFS (CORE:) ---
    if (!LittleFS.begin(true)) {   // true = format if mount fails
      Serial.println("[Kernel] LittleFS mount failed!");
      _shell.printResponse("FS mount failed");
    } else {
      Serial.println("[Kernel] LittleFS mounted as CORE:");
      ensureSystemFolders();
    }

    registerBuiltinCommands();
    _shell.begin();

    Serial.println("[Kernel] " OS_NAME " " OS_VERSION " started");
    return true;
  }

  void loop() {
    _shell.update();
  }

  DisplayManager& display() { return _display; }
  CommandRegistry& commands() { return _commands; }
  Shell& shell() { return _shell; }

private:
  DisplayManager _display;
  CommandRegistry _commands;
  Shell _shell;

  void registerBuiltinCommands() {
    // System
    _commands.registerCommand("help",   "List commands",           cmd_help);
    _commands.registerCommand("clear",  "Clear screen",            cmd_clear);
    _commands.registerCommand("info",   "System information",      cmd_info);
    _commands.registerCommand("echo",   "Print text",              cmd_echo);
    _commands.registerCommand("mode",   "Switch display mode",     cmd_mode);

    // File system (CORE:)
    _commands.registerCommand("ls",     "List files",              cmd_ls);
    _commands.registerCommand("dir",    "List files (alias)",      cmd_ls);
    _commands.registerCommand("cat",    "Show file content",       cmd_cat);
    _commands.registerCommand("write",  "Write text to file",      cmd_write);
    _commands.registerCommand("rm",     "Remove file",             cmd_rm);
    _commands.registerCommand("mkdir",  "Create directory",        cmd_mkdir);
  }

  // -------------------- Commands --------------------

  static void cmd_help(const String& args) {
    String list;
    g_kernel->commands().listCommands(list);
    int start = 0;
    while (start < list.length()) {
      int end = list.indexOf('\n', start);
      if (end == -1) end = list.length();
      g_kernel->shell().printResponse(list.substring(start, end));
      start = end + 1;
    }
  }

  static void cmd_clear(const String& args) {
    g_kernel->display().clearLines();
  }

  static void cmd_info(const String& args) {
    g_kernel->shell().printResponse(String(OS_NAME) + " " + OS_VERSION);
    g_kernel->shell().printResponse("Free heap: " + String(ESP.getFreeHeap()));
    g_kernel->shell().printResponse("CPU: " + String(ESP.getCpuFreqMHz()) + " MHz");

    // FS info
    size_t total = LittleFS.totalBytes();
    size_t used  = LittleFS.usedBytes();
    g_kernel->shell().printResponse("CORE: " + String(used/1024) + "/" + String(total/1024) + " KB");
  }

  static void cmd_echo(const String& args) {
    if (args.length() > 0) {
      g_kernel->shell().printResponse(args);
    }
  }

  static void cmd_mode(const String& args) {
    if (args.length() == 0) {
      DisplayMode m = g_kernel->display().getMode();
      String name = (m == DisplayMode::SHELL) ? "shell" : "status";
      g_kernel->shell().printResponse("Current mode: " + name);
      return;
    }

    if (args.equalsIgnoreCase("shell")) {
      g_kernel->display().setMode(DisplayMode::SHELL);
      g_kernel->shell().printResponse("Switched to SHELL");
    } else if (args.equalsIgnoreCase("status")) {
      g_kernel->display().setMode(DisplayMode::STATUS);
    } else {
      g_kernel->shell().printResponse("Unknown mode");
    }
  }

  // ---------- File commands ----------

  static void cmd_ls(const String& args) {
    String path = args.length() > 0 ? args : "/";
    if (!path.startsWith("/")) path = "/" + path;

    File root = LittleFS.open(path);
    if (!root || !root.isDirectory()) {
      g_kernel->shell().printResponse("Not a directory");
      return;
    }

    File file = root.openNextFile();
    if (!file) {
      g_kernel->shell().printResponse("(empty)");
      return;
    }

    while (file) {
      String name = file.name();
      // LittleFS sometimes returns full path, берём только имя
      int lastSlash = name.lastIndexOf('/');
      if (lastSlash >= 0) name = name.substring(lastSlash + 1);

      if (file.isDirectory()) {
        g_kernel->shell().printResponse("[DIR] " + name);
      } else {
        g_kernel->shell().printResponse(name + " " + String(file.size()) + "b");
      }
      file = root.openNextFile();
    }
  }

  static void cmd_cat(const String& args) {
    if (args.length() == 0) {
      g_kernel->shell().printResponse("Usage: cat <file>");
      return;
    }

    String path = args;
    if (!path.startsWith("/")) path = "/" + path;

    File file = LittleFS.open(path, "r");
    if (!file || file.isDirectory()) {
      g_kernel->shell().printResponse("File not found");
      return;
    }

    // Читаем построчно, чтобы не переполнить экран
    while (file.available()) {
      String line = file.readStringUntil('\n');
      line.trim();
      if (line.length() > 0) {
        g_kernel->shell().printResponse(line);
      }
    }
    file.close();
  }

  static void cmd_write(const String& args) {
    // Формат: write <filename> <text>
    int space = args.indexOf(' ');
    if (space <= 0) {
      g_kernel->shell().printResponse("Usage: write <file> <text>");
      return;
    }

    String filename = args.substring(0, space);
    String text = args.substring(space + 1);

    if (!filename.startsWith("/")) filename = "/" + filename;

    File file = LittleFS.open(filename, "w");
    if (!file) {
      g_kernel->shell().printResponse("Cannot write file");
      return;
    }

    file.println(text);
    file.close();
    g_kernel->shell().printResponse("OK");
  }

  static void cmd_rm(const String& args) {
    if (args.length() == 0) {
      g_kernel->shell().printResponse("Usage: rm <file>");
      return;
    }

    String path = args;
    if (!path.startsWith("/")) path = "/" + path;

    if (LittleFS.remove(path)) {
      g_kernel->shell().printResponse("Deleted");
    } else {
      g_kernel->shell().printResponse("Failed");
    }
  }

  static void cmd_mkdir(const String& args) {
    if (args.length() == 0) {
      g_kernel->shell().printResponse("Usage: mkdir <dir>");
      return;
    }

    String path = args;
    if (!path.startsWith("/")) path = "/" + path;

    if (LittleFS.mkdir(path)) {
      g_kernel->shell().printResponse("Created");
    } else {
      g_kernel->shell().printResponse("Failed");
    }
  }
};

// ====================== ENTRY ======================

Kernel kernel;

void setup() {
  kernel.begin();
}

void loop() {
  kernel.loop();
}
