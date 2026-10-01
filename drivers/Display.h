#ifndef ESPOS_DISPLAY_H
#define ESPOS_DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../config/Config.h"

enum class DisplayMode {
  SHELL,
  STATUS,
  // MONITOR,   // будущий режим
  // SETTINGS
};

class DisplayManager {
public:
  DisplayManager();

  bool begin();
  void clear();
  void setMode(DisplayMode mode);
  DisplayMode getMode() const;

  // Работа со строками (для Shell)
  void printLine(const String& text);
  void clearLines();
  void redraw();

  // Прямой доступ к дисплею (если нужно)
  Adafruit_SSD1306& raw();

private:
  Adafruit_SSD1306 _display;
  DisplayMode _mode = DisplayMode::SHELL;

  String _lines[MAX_LINES];
  int _lineCount = 0;

  void drawShell();
  void drawStatus();
};

#endif // ESPOS_DISPLAY_H
