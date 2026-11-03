#ifndef ESPOS_KERNEL_H
#define ESPOS_KERNEL_H

#include <Arduino.h>
#include "../drivers/Display.h"
#include "../drivers/Touch.h"
#include "../services/Storage.h"
#include "../services/NetManager.h"
#include "../services/Shell.h"

class Kernel {
public:
  Kernel();

  bool begin();
  void loop();

  DisplayManager& display() { return _display; }
  TouchManager& touch() { return _touch; }
  StorageManager& storage() { return _storage; }
  EspNetManager& network() { return _network; }
  Shell& shell() { return _shell; }

private:
  DisplayManager _display;
  TouchManager _touch;
  StorageManager _storage;
  EspNetManager _network;
  Shell _shell;
};

extern Kernel* g_kernel;

#endif // ESPOS_KERNEL_H
