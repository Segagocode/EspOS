#include "CommandRegistry.h"

void CommandRegistry::registerCommand(const char* name, const char* description, CommandHandler handler) {
  if (_count >= MAX_COMMANDS) return;

  _commands[_count].name = name;
  _commands[_count].description = description;
  _commands[_count].handler = handler;
  _count++;
}

int CommandRegistry::findCommand(const String& name) {
  for (int i = 0; i < _count; i++) {
    if (name.equalsIgnoreCase(_commands[i].name)) {
      return i;
    }
  }
  return -1;
}

bool CommandRegistry::execute(const String& input) {
  String cmd = input;
  cmd.trim();
  if (cmd.length() == 0) return false;

  // Разделяем команду и аргументы
  int spaceIndex = cmd.indexOf(' ');
  String name;
  String args = "";

  if (spaceIndex == -1) {
    name = cmd;
  } else {
    name = cmd.substring(0, spaceIndex);
    args = cmd.substring(spaceIndex + 1);
    args.trim();
  }

  int idx = findCommand(name);
  if (idx == -1) {
    return false;
  }

  _commands[idx].handler(args);
  return true;
}

void CommandRegistry::listCommands(String* output, int /*maxLen*/) {
  *output = "";
  for (int i = 0; i < _count; i++) {
    *output += String(_commands[i].name);
    *output += " - ";
    *output += _commands[i].description;
    if (i < _count - 1) *output += "\n";
  }
}
