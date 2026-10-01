#ifndef ESPOS_KERNEL_H
#define ESPOS_KERNEL_H

#include <Arduino.h>
#include "../drivers/Display.h"
#include "../services/CommandRegistry.h"
#include "../services/Shell.h"

class Kernel {
public:
  Kernel();

  bool begin();
  void loop();

  // Доступ к основным сервисам
  DisplayManager& display();
  CommandRegistry& commands();
  Shell& shell();

private:
  DisplayManager _display;
  CommandRegistry _commands;
  Shell _shell;

  void registerBuiltinCommands();
};

// Глобальный экземпляр (удобно для команд)
extern Kernel* g_kernel;

#endif // ESPOS_KERNEL_H
