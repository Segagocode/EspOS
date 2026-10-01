#include "Shell.h"
#include "../config/Config.h"

Shell::Shell(DisplayManager& display, CommandRegistry& registry)
  : _display(display), _registry(registry) {
}

void Shell::begin() {
  _display.setMode(DisplayMode::SHELL);
  _display.clearLines();
  printPrompt();
}

void Shell::printPrompt() {
  _display.printLine("C:\\>");
  _promptShown = true;
}

void Shell::printResponse(const String& text) {
  _display.printLine(text);
}

void Shell::update() {
  while (Serial.available()) {
    char c = Serial.read();

    // Игнорируем \r
    if (c == '\r') continue;

    if (c == '\n') {
      if (_currentLine.length() > 0) {
        // Показываем введённую команду
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
      // Обычный символ
      if (_currentLine.length() < MAX_LINE_LENGTH - 4) {
        _currentLine += c;
      }
    }
  }
}

void Shell::processLine(const String& line) {
  if (!_registry.execute(line)) {
    handleUnknown(line);
  }
}

void Shell::handleUnknown(const String& cmd) {
  _display.printLine("Unknown command");
}
