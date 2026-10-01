#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define MAX_LINES 8
String screenLines[MAX_LINES];
int lineCount = 0;

String line = "";

void printLine(String text) {
  if (lineCount < MAX_LINES) {
    screenLines[lineCount] = text;
    lineCount++;
  } else {
    for (int i = 1; i < MAX_LINES; i++) {
      screenLines[i - 1] = screenLines[i];
    }
    screenLines[MAX_LINES - 1] = text;
  }
  redraw();
}

void redraw() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  for (int i = 0; i < lineCount; i++) {
    display.println(screenLines[i]);
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  printLine("C:\\>");
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();

    if (c == '\r') {
      return;
    }

    if (c == '\n') {
      if (line == "dir") {
        printLine("C:\\> " + line);
        printLine("Disk empty");
      } else if (line.length() > 0) {
        printLine("C:\\> " + line);
        printLine("Unknown command");
      }
      line = "";
      printLine("C:\\>");
    } else {
      line += c;
    }
  }
}
