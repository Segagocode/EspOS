#ifndef ESPOS_SHELL_H
#define ESPOS_SHELL_H

#include <Arduino.h>
#include "../drivers/Display.h"
#include "../drivers/Touch.h"
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

#endif // ESPOS_SHELL_H
