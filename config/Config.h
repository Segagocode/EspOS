#ifndef ESPOS_CONFIG_H
#define ESPOS_CONFIG_H

// ====================== Display ======================
#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT       64
#define OLED_ADDRESS        0x3C
#define OLED_RESET_PIN      -1

#define MAX_LINES           8
#define MAX_LINE_LENGTH     21   // 128px / 6px font ≈ 21 символов

// ====================== Serial =======================
#define SERIAL_BAUD         115200

// ====================== Optional Buttons =============
// Раскомментируй, если подключил кнопки
// #define USE_BUTTONS
// #define BTN_MODE_PIN       0     // BOOT кнопка на многих ESP32
// #define BTN_OK_PIN         2

// ====================== System =======================
#define OS_NAME             "EspOS"
#define OS_VERSION          "0.1.0-alpha"

#endif // ESPOS_CONFIG_H
