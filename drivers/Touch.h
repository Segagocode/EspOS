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

  TouchAction update(DisplayManager& display, String& inputBuffer, String& submittedCmd, const String& cwd);

  void setCalibration(int minX, int maxX, int minY, int maxY);

private:
  XPT2046_Touchscreen _ts;
  unsigned long _lastTouchTime = 0;

  int _calMinX = 200;
  int _calMaxX = 3800;
  int _calMinY = 200;
  int _calMaxY = 3800;
};

#endif // ESPOS_TOUCH_H
