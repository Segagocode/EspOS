#ifndef ESPOS_DISPLAY_H
#define ESPOS_DISPLAY_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "../config/Config.h"

enum class DisplayMode {
  SHELL,
  STATUS,
  RAW_GRAPHICS
};

class DisplayManager {
public:
  DisplayManager();

  bool begin();
  void clear();
  void print(const String& text, uint16_t color = COL_TEXT);
  void prompt(const String& cwd);
  void drawInputLine(const String& inputBuffer);
  void drawKeyboard();
  void toggleKeyboard(const String& inputBuffer, const String& cwd);
  void redrawAll(const String& inputBuffer);

  bool isKeyboardVisible() const { return _kbdVisible; }
  void setKeyboardVisible(bool visible) { _kbdVisible = visible; }

  DisplayMode getMode() const { return _mode; }
  void setMode(DisplayMode mode) { _mode = mode; }

  Adafruit_ILI9341& raw() { return _tft; }

  // Keyboard layout constants
  static const char* kbdRows[5];
  static const int kbdRowLens[5];

private:
  Adafruit_ILI9341 _tft;
  DisplayMode _mode = DisplayMode::SHELL;
  bool _kbdVisible = false;

  String _lines[MAX_LINES];
  uint16_t _lineColors[MAX_LINES];
  int _lineCount = 0;

  void redrawTextArea();
};

#endif // ESPOS_DISPLAY_H
