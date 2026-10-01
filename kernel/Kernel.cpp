#include "Kernel.h"
#include "../config/Config.h"

Kernel* g_kernel = nullptr;

// ================== Builtin command handlers ==================

void cmd_help(const String& args) {
  String list;
  g_kernel->commands().listCommands(&list, 0);
  // Разбиваем по строкам и выводим
  int start = 0;
  while (start < list.length()) {
    int end = list.indexOf('\n', start);
    if (end == -1) end = list.length();
    g_kernel->shell().printResponse(list.substring(start, end));
    start = end + 1;
  }
}

void cmd_clear(const String& args) {
  g_kernel->display().clearLines();
}

void cmd_dir(const String& args) {
  g_kernel->shell().printResponse("Disk empty");
}

void cmd_info(const String& args) {
  g_kernel->shell().printResponse(String(OS_NAME) + " " + OS_VERSION);
  g_kernel->shell().printResponse("Free heap: " + String(ESP.getFreeHeap()));
  g_kernel->shell().printResponse("CPU: " + String(ESP.getCpuFreqMHz()) + " MHz");
}

void cmd_echo(const String& args) {
  if (args.length() > 0) {
    g_kernel->shell().printResponse(args);
  }
}

void cmd_mode(const String& args) {
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
    // В status режиме шелл не рисует prompt
  } else {
    g_kernel->shell().printResponse("Unknown mode");
  }
}

// ================== Kernel ==================

Kernel::Kernel()
  : _shell(_display, _commands) {
  g_kernel = this;
}

bool Kernel::begin() {
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

void Kernel::loop() {
  _shell.update();

  // Здесь позже будут другие задачи
}

void Kernel::registerBuiltinCommands() {
  _commands.registerCommand("help",  "List available commands", cmd_help);
  _commands.registerCommand("clear", "Clear the screen",        cmd_clear);
  _commands.registerCommand("dir",   "List directory",          cmd_dir);
  _commands.registerCommand("info",  "System information",      cmd_info);
  _commands.registerCommand("echo",  "Print text",              cmd_echo);
  _commands.registerCommand("mode",  "Switch display mode",     cmd_mode);
}

DisplayManager& Kernel::display() { return _display; }
CommandRegistry& Kernel::commands() { return _commands; }
Shell& Kernel::shell() { return _shell; }
