#ifndef ESPOS_SHELL_H
#define ESPOS_SHELL_H

#include <Arduino.h>
#include "../drivers/Display.h"
#include "CommandRegistry.h"

class Shell {
public:
  Shell(DisplayManager& display, CommandRegistry& registry);

  void begin();
  void update();               // Вызывать в loop()
  void printPrompt();
  void printResponse(const String& text);

private:
  DisplayManager& _display;
  CommandRegistry& _registry;

  String _currentLine;
  bool _promptShown = false;

  void processLine(const String& line);
  void handleUnknown(const String& cmd);
};

#endif // ESPOS_SHELL_H
