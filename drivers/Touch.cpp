#include "Touch.h"

TouchManager::TouchManager()
  : _ts(TOUCH_CS, TOUCH_IRQ) {
}

bool TouchManager::begin() {
  _ts.begin();
  _ts.setRotation(0);
  return true;
}

bool TouchManager::isTouched() {
  return _ts.touched();
}

bool TouchManager::getCoordinates(int& x, int& y) {
  if (!_ts.touched()) return false;
  TS_Point p = _ts.getPoint();
  x = map(p.x, _calMinX, _calMaxX, 0, 240);
  y = map(p.y, _calMinY, _calMaxY, 0, 320);
  if (x < 0) x = 0;
  if (x > 239) x = 239;
  if (y < 0) y = 0;
  if (y > 319) y = 319;
  return true;
}

void TouchManager::setCalibration(int minX, int maxX, int minY, int maxY) {
  _calMinX = minX;
  _calMaxX = maxX;
  _calMinY = minY;
  _calMaxY = maxY;
}

TouchAction TouchManager::update(DisplayManager& display, String& inputBuffer, String& submittedCmd, const String& cwd) {
  if (millis() - _lastTouchTime < 220) return TouchAction::NONE;
  if (!_ts.touched()) return TouchAction::NONE;

  int x = 0, y = 0;
  if (!getCoordinates(x, y)) return TouchAction::NONE;
  _lastTouchTime = millis();

  // If keyboard is hidden
  if (!display.isKeyboardVisible()) {
    if (y > 270) {
      display.toggleKeyboard(inputBuffer, cwd);
      return TouchAction::KBD_TOGGLED;
    }
    return TouchAction::RAW_TAP;
  }

  int startY = 190;

  // DEL button (x > 170 && y > startY + 95)
  if (x > 170 && y > startY + 95) {
    if (inputBuffer.length() > 0) {
      inputBuffer.remove(inputBuffer.length() - 1);
      display.drawInputLine(inputBuffer);
    }
    return TouchAction::DEL_TYPED;
  }

  // ENTER button (x < 65 && y > startY + 95)
  if (x < 65 && y > startY + 95) {
    display.print("CORE:\\> " + inputBuffer, COL_PROMPT);
    submittedCmd = inputBuffer;
    inputBuffer = "";
    display.drawInputLine(inputBuffer);
    return TouchAction::COMMAND_SUBMITTED;
  }

  // Key rows
  for (int r = 0; r < 5; r++) {
    int keyW = (r == 4) ? 120 : 22;
    int startX = (r == 4) ? 60 : 4;
    int ky = startY + 4 + r * 24;

    if (y >= ky - 3 && y <= ky + 23) {
      for (int c = 0; c < DisplayManager::kbdRowLens[r]; c++) {
        int kx = startX + c * (keyW + 2);
        if (x >= kx - 3 && x <= kx + keyW + 3) {
          if (r == 4) {
            inputBuffer += ' ';
          } else {
            inputBuffer += DisplayManager::kbdRows[r][c];
          }
          display.drawInputLine(inputBuffer);
          return TouchAction::CHAR_TYPED;
        }
      }
    }
  }

  return TouchAction::NONE;
}
