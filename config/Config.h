#ifndef ESPOS_CONFIG_H
#define ESPOS_CONFIG_H

#include <Arduino.h>
#include <Adafruit_ILI9341.h>

// ====================== PINS ======================
// Display (ILI9341 SPI - Hardware VSPI)
#define TFT_CS          15
#define TFT_DC          2
#define TFT_RST         4
#define SPI_SCK         18
#define SPI_MOSI        23
#define SPI_MISO        19

// Touchscreen (XPT2046 SPI - Hardware HSPI, dedicated bus)
#define TOUCH_CS        33
#define TOUCH_CLK       25
#define TOUCH_MOSI      32
#define TOUCH_MISO      35
#define TOUCH_IRQ       27  // Hardware touch detector with internal INPUT_PULLUP

// ====================== UI CONFIG ======================
#define MAX_LINES       14
#define LINE_HEIGHT     12
#define MAX_LINE_LEN    38
#define INPUT_Y         172

// ====================== COLORS ======================
#define COL_BG          ILI9341_BLACK
#define COL_TEXT        ILI9341_WHITE
#define COL_PROMPT      ILI9341_CYAN
#define COL_ERROR       ILI9341_RED
#define COL_OK          ILI9341_GREEN
#define COL_DIR         ILI9341_YELLOW
#define COL_WARN        ILI9341_ORANGE
#define COL_MAGENTA     ILI9341_MAGENTA
#define COL_KBD         ILI9341_DARKGREY
#define COL_KBD_KEY     ILI9341_LIGHTGREY
#define COL_KBD_TEXT    ILI9341_BLACK
#define COL_BAR_BG      ILI9341_DARKGREY

// ====================== SYSTEM ======================
#define SERIAL_BAUD     115200
#define OS_NAME         "EspOS"
#define OS_VERSION      "0.5.0-marauder"

#endif // ESPOS_CONFIG_H
