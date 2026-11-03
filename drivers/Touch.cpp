#include "Touch.h"

TouchManager::TouchManager()
  : _touchSPI(HSPI), _ts(TOUCH_CS) {
}

bool TouchManager::begin() {
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);

  pinMode(TOUCH_IRQ, INPUT_PULLUP);

  _touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  _ts.begin(_touchSPI);
  _ts.setRotation(_rotation);
  return true;
}

void TouchManager::setRotation(uint8_t rot) {
  _rotation = rot % 4;
  _ts.setRotation(_rotation);
}

bool TouchManager::probeSPI(uint16_t& z1, uint16_t& z2, uint16_t& rawX, uint16_t& rawY) {
  _touchSPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  digitalWrite(TOUCH_CS, LOW);

  _touchSPI.transfer(0xB1);
  z1 = _touchSPI.transfer16(0xC1) >> 3;
  z2 = _touchSPI.transfer16(0x91) >> 3;
  rawX = _touchSPI.transfer16(0xD1) >> 3;
  rawY = _touchSPI.transfer16(0xD0) >> 3;
  _touchSPI.transfer16(0);

  digitalWrite(TOUCH_CS, HIGH);
  _touchSPI.endTransaction();
  return true;
}

bool TouchManager::isTouched() {
  return (digitalRead(TOUCH_IRQ) == LOW);
}

bool TouchManager::getRawPoint(TS_Point& p) {
  // Hardware IRQ: pin goes LOW only upon real physical contact
  if (digitalRead(TOUCH_IRQ) == HIGH) {
    return false;
  }

  p = _ts.getPoint();
  return true;
}

bool TouchManager::getCoordinates(int& x, int& y) {
  TS_Point p;
  if (!getRawPoint(p)) return false;

  // Inverted X axis: right side is high rawX, left is low rawX
  x = map(p.x, _calMaxX, _calMinX, 0, 240);
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
  if (millis() - _lastTouchTime < 180) return TouchAction::NONE;

  int x = 0, y = 0;
  if (!getCoordinates(x, y)) return TouchAction::NONE;
  _lastTouchTime = millis();

  Serial.printf("[Touch] Tap at X=%d, Y=%d (kbd=%s)\n", x, y, display.isKeyboardVisible() ? "ON" : "OFF");

  // If keyboard is hidden, tap anywhere in bottom area to open it
  if (!display.isKeyboardVisible()) {
    if (y > 180) {
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
