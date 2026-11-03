#include "Kernel.h"
#include "../config/Config.h"

Kernel* g_kernel = nullptr;

Kernel::Kernel()
  : _shell(_display, _touch, _storage, _network) {
  g_kernel = this;
}

bool Kernel::begin() {
  Serial.begin(SERIAL_BAUD);
  delay(300);

  _display.begin();
  _touch.begin();

  if (!_storage.begin()) {
    _display.print("LittleFS failed!", COL_ERROR);
  }

  _display.print(String(OS_NAME) + " " + OS_VERSION, COL_PROMPT);
  _display.print("Marauder tools ready", COL_OK);

  _network.tryAutoConnect(_display);
  _shell.begin();

  Serial.println("[Kernel] " OS_NAME " " OS_VERSION " started");
  return true;
}

void Kernel::loop() {
  _shell.update();
}
