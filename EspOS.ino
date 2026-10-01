/*
 * EspOS - Minimal modular OS for ESP32
 * Stage 1: Kernel + Display + Command System + Shell
 *
 * Основной файл для удобной компиляции в ArduinoDroid / Arduino IDE.
 * Вся логика вынесена в модули.
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
