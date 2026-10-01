#ifndef ESPOS_COMMAND_REGISTRY_H
#define ESPOS_COMMAND_REGISTRY_H

#include <Arduino.h>

typedef void (*CommandHandler)(const String& args);

struct Command {
  const char* name;
  const char* description;
  CommandHandler handler;
};

class CommandRegistry {
public:
  static const int MAX_COMMANDS = 16;

  void registerCommand(const char* name, const char* description, CommandHandler handler);
  bool execute(const String& input);
  void listCommands(String* output, int maxLen);

private:
  Command _commands[MAX_COMMANDS];
  int _count = 0;

  int findCommand(const String& name);
};

#endif // ESPOS_COMMAND_REGISTRY_H
