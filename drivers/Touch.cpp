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

uint16_t TouchManager::readADC(uint8_t cmd) {
  _touchSPI.transfer(cmd);
  uint8_t h = _touchSPI.transfer(0x00);
  uint8_t l = _touchSPI.transfer(0x00);
  return ((uint16_t)(h & 0x7F) << 5) | (l >> 3);
}

bool TouchManager::probeSPI(uint16_t& z1, uint16_t& z2, uint16_t& rawX, uint16_t& rawY) {
  _touchSPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  digitalWrite(TOUCH_CS, LOW);

  z1 = readADC(0xB1);
  z2 = readADC(0xC1);
  rawX = readADC(0xD1);
  rawY = readADC(0x91);

  _touchSPI.transfer(0xD0);
  _touchSPI.transfer(0x00);
  _touchSPI.transfer(0x00);

  digitalWrite(TOUCH_CS, HIGH);
  _touchSPI.endTransaction();
  return true;
}

static inline uint16_t median3(uint16_t a, uint16_t b, uint16_t c) {
  if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
  if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
  return c;
}

bool TouchManager::getRawPoint(TS_Point& p) {
  _touchSPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  digitalWrite(TOUCH_CS, LOW);

  // 1. Measure Z1 and Z2
  uint16_t z1 = readADC(0xB1);
  uint16_t z2 = readADC(0xC1);

  // Filter 1: First sample must exceed idle noise floor (idle is z1 ~ 25..45, z2 ~ 8..25)
  if (z1 < 58 || z2 < 35) {
    _touchSPI.transfer(0xD0);
    _touchSPI.transfer(0x00);
    _touchSPI.transfer(0x00);
    digitalWrite(TOUCH_CS, HIGH);
    _touchSPI.endTransaction();
    return false;
  }

  // Filter 2: Second sample 3ms later to eliminate transient capacitive / backlight spikes
  delay(3);
  uint16_t z1_b = readADC(0xB1);
  uint16_t z2_b = readADC(0xC1);
  if (z1_b < 50 || z2_b < 28) {
    _touchSPI.transfer(0xD0);
    _touchSPI.transfer(0x00);
    _touchSPI.transfer(0x00);
    digitalWrite(TOUCH_CS, HIGH);
    _touchSPI.endTransaction();
    return false;
  }

  // 2. Read X and Y (3 samples for median filtering to eliminate jitter)
  uint16_t xs[3];
  uint16_t ys[3];
  for (int i = 0; i < 3; i++) {
    xs[i] = readADC(0xD1); // X
    ys[i] = readADC(0x91); // Y
  }

  // Power down ADC and enable PENIRQ
  _touchSPI.transfer(0xD0);
  _touchSPI.transfer(0x00);
  _touchSPI.transfer(0x00);

  digitalWrite(TOUCH_CS, HIGH);
  _touchSPI.endTransaction();

  uint16_t rx = median3(xs[0], xs[1], xs[2]);
  uint16_t ry = median3(ys[0], ys[1], ys[2]);

  // Coordinate validity bounds check
  if (rx < 100 || rx > 4000 || ry < 100 || ry > 4000) {
    return false;
  }

  p.x = rx;
  p.y = ry;
  p.z = z1_b;

  Serial.printf("[DEBUG HIT] z1=%u z2=%u rawX=%u rawY=%u\n", z1_b, z2_b, rx, ry);
  return true;
}

bool TouchManager::isTouched() {
  TS_Point p;
  return getRawPoint(p);
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
  TS_Point p;
  if (!getRawPoint(p)) return TouchAction::NONE;
  _lastTouchTime = millis();

  x = map(p.x, _calMaxX, _calMinX, 0, 240);
  y = map(p.y, _calMinY, _calMaxY, 0, 320);
  if (x < 0) x = 0;
  if (x > 239) x = 239;
  if (y < 0) y = 0;
  if (y > 319) y = 319;

  Serial.printf("[Touch] Tap at X=%d, Y=%d (rawX=%d, rawY=%d, z=%d)\n", x, y, p.x, p.y, p.z);

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
