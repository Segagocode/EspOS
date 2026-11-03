#ifndef ESPOS_TOUCH_H
#define ESPOS_TOUCH_H

#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "../config/Config.h"
#include "Display.h"

enum class TouchAction {
  NONE,
  CHAR_TYPED,
  DEL_TYPED,
  COMMAND_SUBMITTED,
  KBD_TOGGLED,
  RAW_TAP
};

class TouchManager {
public:
  TouchManager();

  bool begin();
  bool isTouched();
  bool getCoordinates(int& x, int& y);
  bool getRawPoint(TS_Point& p);

  // Direct SPI probe for hardware diagnostics
  bool probeSPI(uint16_t& z1, uint16_t& z2, uint16_t& rawX, uint16_t& rawY);

  TouchAction update(DisplayManager& display, String& inputBuffer, String& submittedCmd, const String& cwd);

  void setCalibration(int minX, int maxX, int minY, int maxY);
  void setRotation(uint8_t rot);
  uint8_t getRotation() const { return _rotation; }

  int getCalMinX() const { return _calMinX; }
  int getCalMaxX() const { return _calMaxX; }
  int getCalMinY() const { return _calMinY; }
  int getCalMaxY() const { return _calMaxY; }

private:
  SPIClass _touchSPI;
  XPT2046_Touchscreen _ts;
  unsigned long _lastTouchTime = 0;
  uint8_t _rotation = 0;

  int _calMinX = 200;
  int _calMaxX = 3800;
  int _calMinY = 200;
  int _calMaxY = 3800;
};

#endif // ESPOS_TOUCH_H
