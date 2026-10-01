/*
 * EspOS - Single-file version for ArduinoDroid
 * Stage 1: Kernel + Display Manager + Command System + Shell
 *
 * Этот файл специально сделан монолитным,
 * чтобы без проблем компилироваться в ArduinoDroid.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ====================== CONFIG ======================
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_ADDRESS        0x3C
#define OLED_RESET_PIN      -1

#define MAX_LINES           8
#define MAX_LINE_LENGTH     21

#define SERIAL_BAUD         115200

#define OS_NAME             "EspOS"
#define OS_VERSION          "0.1.0-alpha"

// ====================== FORWARD DECLARATIONS ======================
class Kernel;
Kernel* g_kernel = nullptr;   // Объявляем заранее!

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
    _display.println("Chip: ESP32");
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
  static const int MAX_COMMANDS = 16;

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
    _display.printLine("C:\\>");
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
          _display.printLine("C:\\> " + _currentLine);
          processLine(_currentLine);
          _currentLine = "";
        }
        printPrompt();
      } else if (c == 8 || c == 127) { // Backspace
        if (_currentLine.length() > 0) {
          _currentLine.remove(_currentLine.length() - 1);
        }
      } else {
        if (_currentLine.length() < MAX_LINE_LENGTH - 4) {
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

// ====================== KERNEL ======================

class Kernel {
public:
  Kernel() : _shell(_display, _commands) {
    g_kernel = this;
  }

  bool begin() {
    Serial.begin(SERIAL_BAUD);
    delay(100);

    if (!_display.begin()) {
      Serial.println("[Kernel] Display init failed!");
      return false;
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
    _commands.registerCommand("help",  "List available commands", cmd_help);
    _commands.registerCommand("clear", "Clear the screen",        cmd_clear);
    _commands.registerCommand("dir",   "List directory",          cmd_dir);
    _commands.registerCommand("info",  "System information",      cmd_info);
    _commands.registerCommand("echo",  "Print text",              cmd_echo);
    _commands.registerCommand("mode",  "Switch display mode",     cmd_mode);
  }

  // ---- Builtin commands ----
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

  static void cmd_dir(const String& args) {
    g_kernel->shell().printResponse("Disk empty");
  }

  static void cmd_info(const String& args) {
    g_kernel->shell().printResponse(String(OS_NAME) + " " + OS_VERSION);
    g_kernel->shell().printResponse("Free heap: " + String(ESP.getFreeHeap()));
    g_kernel->shell().printResponse("CPU: " + String(ESP.getCpuFreqMHz()) + " MHz");
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
};

// ====================== ARDUINO ENTRY ======================

Kernel kernel;

void setup() {
  kernel.begin();
}

void loop() {
  kernel.loop();
}
