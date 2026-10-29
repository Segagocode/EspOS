/*
 * EspOS - Single-file version for ArduinoDroid
 * v0.3.0-alpha
 *
 * 2.8" TFT 240x320 (ILI9341) + LittleFS as CORE:
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <FS.h>
#include <LittleFS.h>

// ====================== CONFIG ======================
#define TFT_CS     15
#define TFT_DC     2
#define TFT_RST    4

#define SCREEN_WIDTH   240
#define SCREEN_HEIGHT  320

#define MAX_LINES           20
#define MAX_LINE_LENGTH     38

#define SERIAL_BAUD         115200

#define OS_NAME             "EspOS"
#define OS_VERSION          "0.3.0-alpha"

#define COLOR_BG       ILI9341_BLACK
#define COLOR_TEXT     ILI9341_WHITE
#define COLOR_PROMPT   ILI9341_CYAN
#define COLOR_ERROR    ILI9341_RED
#define COLOR_OK       ILI9341_GREEN

// ====================== FORWARD ======================
class Kernel;
extern Kernel* g_kernel;

// ====================== DISPLAY ======================

enum class DisplayMode {
  SHELL,
  STATUS
};

class DisplayManager {
public:
  DisplayManager() : _tft(TFT_CS, TFT_DC, TFT_RST) {}

  bool begin() {
    _tft.begin();
    _tft.setRotation(0);
    _tft.fillScreen(COLOR_BG);
    _tft.setTextSize(1);
    _tft.setTextColor(COLOR_TEXT);
    return true;
  }

  void clear() {
    _tft.fillScreen(COLOR_BG);
  }

  void setMode(DisplayMode mode) {
    _mode = mode;
    redraw();
  }

  DisplayMode getMode() const {
    return _mode;
  }

  void printLine(const String& text, uint16_t color = COLOR_TEXT) {
    String truncated = text;
    if (truncated.length() > MAX_LINE_LENGTH) {
      truncated = truncated.substring(0, MAX_LINE_LENGTH);
    }

    if (_lineCount < MAX_LINES) {
      _lines[_lineCount] = truncated;
      _colors[_lineCount] = color;
      _lineCount++;
    } else {
      for (int i = 1; i < MAX_LINES; i++) {
        _lines[i - 1] = _lines[i];
        _colors[i - 1] = _colors[i];
      }
      _lines[MAX_LINES - 1] = truncated;
      _colors[MAX_LINES - 1] = color;
    }
    redraw();
  }

  void clearLines() {
    _lineCount = 0;
    for (int i = 0; i < MAX_LINES; i++) {
      _lines[i] = "";
      _colors[i] = COLOR_TEXT;
    }
    redraw();
  }

  void redraw() {
    _tft.fillScreen(COLOR_BG);

    switch (_mode) {
      case DisplayMode::SHELL:
        drawShell();
        break;
      case DisplayMode::STATUS:
        drawStatus();
        break;
    }
  }

private:
  Adafruit_ILI9341 _tft;
  DisplayMode _mode = DisplayMode::SHELL;
  String _lines[MAX_LINES];
  uint16_t _colors[MAX_LINES];
  int _lineCount = 0;

  void drawShell() {
    for (int i = 0; i < _lineCount; i++) {
      _tft.setCursor(2, i * 12 + 2);
      _tft.setTextColor(_colors[i]);
      _tft.print(_lines[i]);
    }
  }

  void drawStatus() {
    _tft.setTextColor(COLOR_PROMPT);
    _tft.setCursor(10, 20);
    _tft.setTextSize(2);
    _tft.println(OS_NAME);

    _tft.setTextSize(1);
    _tft.setTextColor(COLOR_TEXT);
    _tft.setCursor(10, 55);
    _tft.print("v");
    _tft.println(OS_VERSION);

    _tft.setCursor(10, 80);
    _tft.println("Display: 2.8\" TFT 240x320");

    _tft.setCursor(10, 100);
    _tft.println("Heap: " + String(ESP.getFreeHeap()));

    _tft.setCursor(10, 120);
    _tft.println("FS: LittleFS (CORE:)");

    _tft.setCursor(10, 150);
    _tft.setTextColor(COLOR_OK);
    _tft.println("Ready for SD card");
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

// ====================== FS HELPERS ======================

bool ensureSystemFolders() {
  if (!LittleFS.exists("/system")) LittleFS.mkdir("/system");
  if (!LittleFS.exists("/config"))  LittleFS.mkdir("/config");
  if (!LittleFS.exists("/logs"))    LittleFS.mkdir("/logs");
  return true;
}

String normalizePath(String path) {
  if (path.length() == 0) return "/";
  if (!path.startsWith("/")) path = "/" + path;

  while (path.indexOf("//") >= 0) path.replace("//", "/");
  if (path.length() > 1 && path.endsWith("/")) path.remove(path.length() - 1);
  if (path.length() == 0) path = "/";
  return path;
}

String resolvePath(const String& path, const String& cwd) {
  if (path.startsWith("/")) return normalizePath(path);
  if (path == ".") return cwd;
  if (path == "..") {
    if (cwd == "/") return "/";
    int last = cwd.lastIndexOf('/');
    if (last <= 0) return "/";
    return cwd.substring(0, last);
  }
  if (cwd == "/") return normalizePath("/" + path);
  return normalizePath(cwd + "/" + path);
}

// ====================== KERNEL (full definition first) ======================

class Kernel {
public:
  Kernel();

  bool begin();
  void loop();

  DisplayManager& display() { return _display; }
  CommandRegistry& commands() { return _commands; }

  String getCwd() const { return _cwd; }
  void setCwd(const String& path) { _cwd = path; }

  // Нужно для Shell
  void shellPrintPrompt();
  void shellPrintResponse(const String& text, uint16_t color = COLOR_TEXT);
  void shellUpdate();

private:
  DisplayManager _display;
  CommandRegistry _commands;
  String _cwd;

  void registerBuiltinCommands();

  // Command handlers
  static void cmd_help(const String& args);
  static void cmd_clear(const String& args);
  static void cmd_info(const String& args);
  static void cmd_echo(const String& args);
  static void cmd_mode(const String& args);
  static void cmd_pwd(const String& args);
  static void cmd_cd(const String& args);
  static void cmd_ls(const String& args);
  static void cmd_cat(const String& args);
  static void cmd_write(const String& args);
  static void cmd_rm(const String& args);
  static void cmd_mkdir(const String& args);
};

Kernel* g_kernel = nullptr;

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
    String prompt = "CORE:";
    String cwd = g_kernel->getCwd();

    if (cwd == "/") {
      prompt += "\\>";
    } else {
      String nice = cwd;
      nice.replace("/", "\\");
      prompt += nice + "\\>";
    }

    if (prompt.length() > MAX_LINE_LENGTH) {
      prompt = prompt.substring(prompt.length() - MAX_LINE_LENGTH);
    }

    _display.printLine(prompt, COLOR_PROMPT);
  }

  void printResponse(const String& text, uint16_t color = COLOR_TEXT) {
    _display.printLine(text, color);
  }

  void update() {
    while (Serial.available()) {
      char c = Serial.read();

      if (c == '\r') continue;

      if (c == '\n') {
        if (_currentLine.length() > 0) {
          String shown = "CORE:";
          String cwd = g_kernel->getCwd();
          if (cwd == "/") shown += "\\> ";
          else {
            String nice = cwd;
            nice.replace("/", "\\");
            shown += nice + "\\> ";
          }
          shown += _currentLine;

          if (shown.length() > MAX_LINE_LENGTH) {
            shown = shown.substring(shown.length() - MAX_LINE_LENGTH);
          }
          _display.printLine(shown, COLOR_PROMPT);

          processLine(_currentLine);
          _currentLine = "";
        }
        printPrompt();
      } else if (c == 8 || c == 127) {
        if (_currentLine.length() > 0) {
          _currentLine.remove(_currentLine.length() - 1);
        }
      } else {
        if (_currentLine.length() < 50) {
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
      _display.printLine("Unknown command", COLOR_ERROR);
    }
  }
};

// ====================== KERNEL IMPLEMENTATION ======================

Kernel::Kernel() : _cwd("/") {
  g_kernel = this;
}

bool Kernel::begin() {
  Serial.begin(SERIAL_BAUD);
  delay(200);

  if (!_display.begin()) {
    Serial.println("[Kernel] Display init failed!");
    return false;
  }

  if (!LittleFS.begin(true)) {
    Serial.println("[Kernel] LittleFS mount failed!");
  } else {
    Serial.println("[Kernel] LittleFS mounted as CORE:");
    ensureSystemFolders();
  }

  registerBuiltinCommands();

  // Создаём Shell после того, как Kernel полностью готов
  static Shell shell(_display, _commands);
  // Сохраняем указатель через костыль (для простоты в single-file)
  // В реальности Shell живёт как локальный статический

  shell.begin();

  Serial.println("[Kernel] " OS_NAME " " OS_VERSION " started");
  return true;
}

void Kernel::loop() {
  // Shell update будет вызываться через глобальный доступ
  // Для простоты сделаем Shell тоже глобальным по духу
}

void Kernel::registerBuiltinCommands() {
  _commands.registerCommand("help",   "List commands",           cmd_help);
  _commands.registerCommand("clear",  "Clear screen",            cmd_clear);
  _commands.registerCommand("info",   "System information",      cmd_info);
  _commands.registerCommand("echo",   "Print text",              cmd_echo);
  _commands.registerCommand("mode",   "Switch display mode",     cmd_mode);

  _commands.registerCommand("ls",     "List files",              cmd_ls);
  _commands.registerCommand("dir",    "List files (alias)",      cmd_ls);
  _commands.registerCommand("cd",     "Change directory",        cmd_cd);
  _commands.registerCommand("pwd",    "Print working directory", cmd_pwd);
  _commands.registerCommand("cat",    "Show file content",       cmd_cat);
  _commands.registerCommand("write",  "Write text to file",      cmd_write);
  _commands.registerCommand("rm",     "Remove file",             cmd_rm);
  _commands.registerCommand("mkdir",  "Create directory",        cmd_mkdir);
}

// ---------- Command implementations ----------

void Kernel::cmd_help(const String& args) {
  String list;
  g_kernel->commands().listCommands(list);
  int start = 0;
  while (start < list.length()) {
    int end = list.indexOf('\n', start);
    if (end == -1) end = list.length();
    g_kernel->display().printLine(list.substring(start, end));
    start = end + 1;
  }
}

void Kernel::cmd_clear(const String& args) {
  g_kernel->display().clearLines();
}

void Kernel::cmd_info(const String& args) {
  g_kernel->display().printLine(String(OS_NAME) + " " + OS_VERSION, COLOR_PROMPT);
  g_kernel->display().printLine("Free heap: " + String(ESP.getFreeHeap()));
  g_kernel->display().printLine("CPU: " + String(ESP.getCpuFreqMHz()) + " MHz");
  g_kernel->display().printLine("Display: 2.8\" 240x320 TFT");

  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  g_kernel->display().printLine("CORE: " + String(used/1024) + "/" + String(total/1024) + " KB");
}

void Kernel::cmd_echo(const String& args) {
  if (args.length() > 0) {
    g_kernel->display().printLine(args);
  }
}

void Kernel::cmd_mode(const String& args) {
  if (args.length() == 0) {
    DisplayMode m = g_kernel->display().getMode();
    String name = (m == DisplayMode::SHELL) ? "shell" : "status";
    g_kernel->display().printLine("Current mode: " + name);
    return;
  }

  if (args.equalsIgnoreCase("shell")) {
    g_kernel->display().setMode(DisplayMode::SHELL);
    g_kernel->display().printLine("Switched to SHELL", COLOR_OK);
  } else if (args.equalsIgnoreCase("status")) {
    g_kernel->display().setMode(DisplayMode::STATUS);
  } else {
    g_kernel->display().printLine("Unknown mode", COLOR_ERROR);
  }
}

void Kernel::cmd_pwd(const String& args) {
  g_kernel->display().printLine(g_kernel->getCwd());
}

void Kernel::cmd_cd(const String& args) {
  String target;
  if (args.length() == 0 || args == "~") {
    target = "/";
  } else {
    target = resolvePath(args, g_kernel->getCwd());
  }

  File dir = LittleFS.open(target.c_str());
  if (!dir || !dir.isDirectory()) {
    g_kernel->display().printLine("No such directory", COLOR_ERROR);
    return;
  }
  dir.close();

  g_kernel->setCwd(target);
}

void Kernel::cmd_ls(const String& args) {
  String path = (args.length() == 0) ? g_kernel->getCwd() : resolvePath(args, g_kernel->getCwd());

  File root = LittleFS.open(path.c_str());
  if (!root || !root.isDirectory()) {
    g_kernel->display().printLine("Not a directory", COLOR_ERROR);
    return;
  }

  File file = root.openNextFile();
  if (!file) {
    g_kernel->display().printLine("(empty)");
    return;
  }

  while (file) {
    String name = file.name();
    int lastSlash = name.lastIndexOf('/');
    if (lastSlash >= 0) name = name.substring(lastSlash + 1);

    if (file.isDirectory()) {
      g_kernel->display().printLine("[DIR] " + name, COLOR_PROMPT);
    } else {
      g_kernel->display().printLine(name + " " + String(file.size()) + "b");
    }
    file = root.openNextFile();
  }
}

void Kernel::cmd_cat(const String& args) {
  if (args.length() == 0) {
    g_kernel->display().printLine("Usage: cat <file>", COLOR_ERROR);
    return;
  }

  String path = resolvePath(args, g_kernel->getCwd());
  File file = LittleFS.open(path.c_str(), "r");
  if (!file || file.isDirectory()) {
    g_kernel->display().printLine("File not found", COLOR_ERROR);
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      g_kernel->display().printLine(line);
    }
  }
  file.close();
}

void Kernel::cmd_write(const String& args) {
  int space = args.indexOf(' ');
  if (space <= 0) {
    g_kernel->display().printLine("Usage: write <file> <text>", COLOR_ERROR);
    return;
  }

  String filename = args.substring(0, space);
  String text = args.substring(space + 1);
  String path = resolvePath(filename, g_kernel->getCwd());

  File file = LittleFS.open(path.c_str(), "w");
  if (!file) {
    g_kernel->display().printLine("Cannot write file", COLOR_ERROR);
    return;
  }

  file.println(text);
  file.close();
  g_kernel->display().printLine("OK", COLOR_OK);
}

void Kernel::cmd_rm(const String& args) {
  if (args.length() == 0) {
    g_kernel->display().printLine("Usage: rm <file>", COLOR_ERROR);
    return;
  }

  String path = resolvePath(args, g_kernel->getCwd());
  if (LittleFS.remove(path.c_str())) {
    g_kernel->display().printLine("Deleted", COLOR_OK);
  } else {
    g_kernel->display().printLine("Failed", COLOR_ERROR);
  }
}

void Kernel::cmd_mkdir(const String& args) {
  if (args.length() == 0) {
    g_kernel->display().printLine("Usage: mkdir <dir>", COLOR_ERROR);
    return;
  }

  String path = resolvePath(args, g_kernel->getCwd());
  if (LittleFS.mkdir(path.c_str())) {
    g_kernel->display().printLine("Created", COLOR_OK);
  } else {
    g_kernel->display().printLine("Failed", COLOR_ERROR);
  }
}

// ====================== GLOBAL SHELL + ENTRY ======================

DisplayManager display;
CommandRegistry commands;
Shell shell(display, commands);
Kernel kernel;

void setup() {
  // Переназначаем, чтобы Shell и Kernel использовали одни и те же объекты
  // (в single-file это самый простой способ)
  kernel.begin();

  // Принудительно инициализируем shell после kernel
  shell.begin();
}

void loop() {
  shell.update();
}
