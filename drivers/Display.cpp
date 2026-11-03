#include "Display.h"

const char* DisplayManager::kbdRows[5] = {
  "1234567890",
  "qwertyuiop",
  "asdfghjkl/",
  "zxcvbnm._-",
  " "
};

const int DisplayManager::kbdRowLens[5] = {10, 10, 10, 10, 1};

DisplayManager::DisplayManager()
  : _tft(TFT_CS, TFT_DC, TFT_RST) {
}

bool DisplayManager::begin() {
  _tft.begin();
  _tft.setRotation(0);
  _tft.fillScreen(COL_BG);
  _tft.setTextSize(1);
  return true;
}

void DisplayManager::clear() {
  _tft.fillScreen(COL_BG);
  _lineCount = 0;
}

void DisplayManager::redrawTextArea() {
  int maxY = _kbdVisible ? INPUT_Y : (MAX_LINES * LINE_HEIGHT + 4);
  _tft.fillRect(0, 0, 240, maxY, COL_BG);

  int visible = _kbdVisible ? 13 : MAX_LINES;
  for (int i = 0; i < _lineCount && i < visible; i++) {
    _tft.setCursor(2, i * LINE_HEIGHT + 2);
    _tft.setTextColor(_lineColors[i]);
    _tft.setTextSize(1);
    _tft.print(_lines[i]);
  }
}

void DisplayManager::print(const String& text, uint16_t color) {
  String t = text;
  if ((int)t.length() > MAX_LINE_LEN) {
    t = t.substring(0, MAX_LINE_LEN);
  }

  int visible = _kbdVisible ? 13 : MAX_LINES;

  if (_lineCount >= visible) {
    for (int i = 1; i < visible; i++) {
      _lines[i - 1] = _lines[i];
      _lineColors[i - 1] = _lineColors[i];
    }
    _lineCount = visible - 1;
    redrawTextArea();
  }

  _lines[_lineCount] = t;
  _lineColors[_lineCount] = color;

  int y = _lineCount * LINE_HEIGHT + 2;
  _tft.fillRect(0, y, 240, LINE_HEIGHT, COL_BG);
  _tft.setCursor(2, y);
  _tft.setTextColor(color);
  _tft.setTextSize(1);
  _tft.print(t);

  _lineCount++;
}

void DisplayManager::prompt(const String& cwd) {
  String p = "CORE:";
  if (cwd == "/") {
    p += "\\>";
  } else {
    String nice = cwd;
    nice.replace("/", "\\");
    p += nice + "\\>";
  }
  print(p, COL_PROMPT);
}

void DisplayManager::drawInputLine(const String& inputBuffer) {
  int y = _kbdVisible ? INPUT_Y : (MAX_LINES * LINE_HEIGHT + 2);
  _tft.fillRect(0, y, 240, LINE_HEIGHT, COL_BG);
  _tft.setCursor(2, y);
  _tft.setTextColor(COL_PROMPT);
  _tft.setTextSize(1);

  String preview = "> " + inputBuffer + "_";
  if ((int)preview.length() > MAX_LINE_LEN) {
    preview = preview.substring(preview.length() - MAX_LINE_LEN);
  }
  _tft.print(preview);
}

void DisplayManager::drawKeyboard() {
  int startY = 190;
  _tft.fillRect(0, startY, 240, 130, COL_KBD);

  for (int r = 0; r < 5; r++) {
    int keyW = (r == 4) ? 120 : 22;
    int startX = (r == 4) ? 60 : 4;
    int y = startY + 4 + r * 24;

    for (int c = 0; c < kbdRowLens[r]; c++) {
      int x = startX + c * (keyW + 2);
      _tft.fillRoundRect(x, y, keyW, 20, 3, COL_KBD_KEY);
      _tft.setTextColor(COL_KBD_TEXT);
      _tft.setTextSize(1);
      _tft.setCursor(x + 6, y + 6);
      if (r == 4) {
        _tft.print("SPACE");
      } else {
        _tft.print(kbdRows[r][c]);
      }
    }
  }

  // DEL button
  _tft.fillRoundRect(180, startY + 100, 50, 22, 3, ILI9341_RED);
  _tft.setTextColor(ILI9341_WHITE);
  _tft.setCursor(190, startY + 106);
  _tft.print("DEL");

  // ENT button
  _tft.fillRoundRect(4, startY + 100, 50, 22, 3, ILI9341_GREEN);
  _tft.setCursor(12, startY + 106);
  _tft.print("ENT");
}

void DisplayManager::redrawAll(const String& inputBuffer) {
  _tft.fillScreen(COL_BG);
  redrawTextArea();
  drawInputLine(inputBuffer);
  if (_kbdVisible) {
    drawKeyboard();
  }
}

void DisplayManager::toggleKeyboard(const String& inputBuffer, const String& cwd) {
  _kbdVisible = !_kbdVisible;
  redrawAll(inputBuffer);
  if (!_kbdVisible) {
    prompt(cwd);
    drawInputLine(inputBuffer);
  }
}
