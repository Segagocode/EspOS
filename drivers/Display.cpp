#include "Display.h"

DisplayManager::DisplayManager()
  : _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN) {
}

bool DisplayManager::begin() {
  if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    return false;
  }
  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SSD1306_WHITE);
  _display.display();
  return true;
}

void DisplayManager::clear() {
  _display.clearDisplay();
  _display.display();
}

void DisplayManager::setMode(DisplayMode mode) {
  _mode = mode;
  redraw();
}

DisplayMode DisplayManager::getMode() const {
  return _mode;
}

void DisplayManager::printLine(const String& text) {
  String truncated = text;
  if (truncated.length() > MAX_LINE_LENGTH) {
    truncated = truncated.substring(0, MAX_LINE_LENGTH);
  }

  if (_lineCount < MAX_LINES) {
    _lines[_lineCount] = truncated;
    _lineCount++;
  } else {
    // Сдвиг вверх
    for (int i = 1; i < MAX_LINES; i++) {
      _lines[i - 1] = _lines[i];
    }
    _lines[MAX_LINES - 1] = truncated;
  }
  redraw();
}

void DisplayManager::clearLines() {
  _lineCount = 0;
  for (int i = 0; i < MAX_LINES; i++) {
    _lines[i] = "";
  }
  redraw();
}

void DisplayManager::redraw() {
  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SSD1306_WHITE);
  _display.setCursor(0, 0);

  switch (_mode) {
    case DisplayMode::SHELL:
      drawShell();
      break;
    case DisplayMode::STATUS:
      drawStatus();
      break;
  }

  _display.display();
}

void DisplayManager::drawShell() {
  for (int i = 0; i < _lineCount; i++) {
    _display.setCursor(0, i * 8);
    _display.print(_lines[i]);
  }
}

void DisplayManager::drawStatus() {
  _display.setCursor(0, 0);
  _display.println(OS_NAME);
  _display.print("v");
  _display.println(OS_VERSION);
  _display.println("------------");
  _display.println("Mode: STATUS");
  _display.println("Heap: " + String(ESP.getFreeHeap()));
  _display.println("Chip: ESP32");
}

Adafruit_SSD1306& DisplayManager::raw() {
  return _display;
}
