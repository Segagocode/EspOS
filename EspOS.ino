/*
 * EspOS - Minimal modular OS for ESP32
 * v0.5.0-marauder: Modular Architecture + Marauder Wireless Tools
 */

#include "config/Config.h"
#include "kernel/Kernel.h"

Kernel kernel;

void setup() {
  kernel.begin();
}

void loop() {
  kernel.loop();
}
