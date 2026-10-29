/*
 * EspOS v0.3.1-alpha
 * 2.8" TFT 240x320 (ILI9341)
 * Fixed: flickering + Serial input + default argument error
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <FS.h>
#include <LittleFS.h>

// ====================== PINS ======================
#define TFT_CS     15
#define TFT_DC     2
#define TFT_RST    4

// ====================== CONFIG ======================
#define MAX_LINES        22
#define LINE_HEIGHT      12
#define MAX_LINE_LEN     38

#define SERIAL_BAUD      115200

#define OS_NAME          "EspOS"
#define OS_VERSION       "0.3.1-alpha"

#define COL_BG           ILI9341_BLACK
#define COL_TEXT         ILI9341_WHITE
#define COL_PROMPT       ILI9341_CYAN
#define COL_ERROR        ILI9341_RED
#define COL_OK           ILI9341_GREEN
#define COL_DIR          ILI9341_YELLOW

// ====================== GLOBALS ======================
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

String lines[MAX_LINES];
uint16_t lineColors[MAX_LINES];
int lineCount = 0;

String cwd = "/";
String inputBuffer = "";

// ====================== DISPLAY ======================

void displayClear() {
  tft.fillScreen(COL_BG);
  lineCount = 0;
}

void displayPrint(const String& text, uint16_t color) {
  String t = text;
  if (t.length() > MAX_LINE_LEN) t = t.substring(0, MAX_LINE_LEN);

  if (lineCount >= MAX_LINES) {
    for (int i = 1; i < MAX_LINES; i++) {
      lines[i-1] = lines[i];
      lineColors[i-1] = lineColors[i];
    }
    lineCount = MAX_LINES - 1;
  }

  lines[lineCount] = t;
  lineColors[lineCount] = color;
  lineCount++;

  int y = (lineCount - 1) * LINE_HEIGHT + 2;
  tft.fillRect(0, y, 240, LINE_HEIGHT, COL_BG);
  tft.setCursor(2, y);
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.print(t);
}

void displayPrompt() {
  String p = "CORE:";
  if (cwd == "/") {
    p += "\\>";
  } else {
    String nice = cwd;
    nice.replace("/", "\\");
    p += nice + "\\>";
  }
  displayPrint(p, COL_PROMPT);
}

// ====================== FS ======================

String normalizePath(String path) {
  if (path.length() == 0) return "/";
  if (!path.startsWith("/")) path = "/" + path;
  while (path.indexOf("//") >= 0) path.replace("//", "/");
  if (path.length() > 1 && path.endsWith("/")) path.remove(path.length()-1);
  return path.length() == 0 ? "/" : path;
}

String resolvePath(const String& path) {
  if (path.startsWith("/")) return normalizePath(path);
  if (path == ".") return cwd;
  if (path == "..") {
    if (cwd == "/") return "/";
    int last = cwd.lastIndexOf('/');
    return (last <= 0) ? "/" : cwd.substring(0, last);
  }
  if (cwd == "/") return normalizePath("/" + path);
  return normalizePath(cwd + "/" + path);
}

void ensureFolders() {
  if (!LittleFS.exists("/system")) LittleFS.mkdir("/system");
  if (!LittleFS.exists("/config"))  LittleFS.mkdir("/config");
  if (!LittleFS.exists("/logs"))    LittleFS.mkdir("/logs");
}

// ====================== COMMANDS ======================

void cmd_help() {
  displayPrint("help clear info echo mode", COL_TEXT);
  displayPrint("ls cd pwd cat write rm mkdir", COL_TEXT);
}

void cmd_clear() {
  displayClear();
  displayPrompt();
}

void cmd_info() {
  displayPrint(String(OS_NAME) + " " + OS_VERSION, COL_PROMPT);
  displayPrint("Heap: " + String(ESP.getFreeHeap()), COL_TEXT);
  displayPrint("CPU: " + String(ESP.getCpuFreqMHz()) + " MHz", COL_TEXT);
  displayPrint("Display: 2.8\" 240x320", COL_TEXT);
  size_t total = LittleFS.totalBytes();
  size_t used = LittleFS.usedBytes();
  displayPrint("CORE: " + String(used/1024) + "/" + String(total/1024) + " KB", COL_TEXT);
}

void cmd_echo(const String& args) {
  if (args.length()) displayPrint(args, COL_TEXT);
}

void cmd_mode(const String& args) {
  if (args == "status") {
    displayClear();
    tft.setTextColor(COL_PROMPT);
    tft.setTextSize(2);
    tft.setCursor(10, 30);
    tft.println(OS_NAME);
    tft.setTextSize(1);
    tft.setTextColor(COL_TEXT);
    tft.setCursor(10, 70);
    tft.println("v" + String(OS_VERSION));
    tft.setCursor(10, 100);
    tft.println("2.8\" TFT 240x320");
    tft.setCursor(10, 120);
    tft.println("Heap: " + String(ESP.getFreeHeap()));
    tft.setCursor(10, 150);
    tft.setTextColor(COL_OK);
    tft.println("SD slot ready");
  } else {
    displayClear();
    displayPrompt();
  }
}

void cmd_pwd() {
  displayPrint(cwd, COL_TEXT);
}

void cmd_cd(const String& args) {
  String target = (args.length() == 0 || args == "~") ? "/" : resolvePath(args);
  File dir = LittleFS.open(target.c_str());
  if (!dir || !dir.isDirectory()) {
    displayPrint("No such directory", COL_ERROR);
    return;
  }
  dir.close();
  cwd = target;
}

void cmd_ls(const String& args) {
  String path = args.length() == 0 ? cwd : resolvePath(args);
  File root = LittleFS.open(path.c_str());
  if (!root || !root.isDirectory()) {
    displayPrint("Not a directory", COL_ERROR);
    return;
  }

  File f = root.openNextFile();
  if (!f) {
    displayPrint("(empty)", COL_TEXT);
    return;
  }

  while (f) {
    String name = f.name();
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);

    if (f.isDirectory()) {
      displayPrint("[DIR] " + name, COL_DIR);
    } else {
      displayPrint(name + " " + String(f.size()) + "b", COL_TEXT);
    }
    f = root.openNextFile();
  }
}

void cmd_cat(const String& args) {
  if (args.length() == 0) {
    displayPrint("Usage: cat <file>", COL_ERROR);
    return;
  }
  String path = resolvePath(args);
  File f = LittleFS.open(path.c_str(), "r");
  if (!f || f.isDirectory()) {
    displayPrint("File not found", COL_ERROR);
    return;
  }
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length()) displayPrint(line, COL_TEXT);
  }
  f.close();
}

void cmd_write(const String& args) {
  int sp = args.indexOf(' ');
  if (sp <= 0) {
    displayPrint("Usage: write <file> <text>", COL_ERROR);
    return;
  }
  String name = args.substring(0, sp);
  String text = args.substring(sp + 1);
  String path = resolvePath(name);

  File f = LittleFS.open(path.c_str(), "w");
  if (!f) {
    displayPrint("Cannot write", COL_ERROR);
    return;
  }
  f.println(text);
  f.close();
  displayPrint("OK", COL_OK);
}

void cmd_rm(const String& args) {
  if (args.length() == 0) {
    displayPrint("Usage: rm <file>", COL_ERROR);
    return;
  }
  String path = resolvePath(args);
  if (LittleFS.remove(path.c_str())) {
    displayPrint("Deleted", COL_OK);
  } else {
    displayPrint("Failed", COL_ERROR);
  }
}

void cmd_mkdir(const String& args) {
  if (args.length() == 0) {
    displayPrint("Usage: mkdir <dir>", COL_ERROR);
    return;
  }
  String path = resolvePath(args);
  if (LittleFS.mkdir(path.c_str())) {
    displayPrint("Created", COL_OK);
  } else {
    displayPrint("Failed", COL_ERROR);
  }
}

// ====================== PARSER ======================

void processCommand(String line) {
  line.trim();
  if (line.length() == 0) return;

  int sp = line.indexOf(' ');
  String cmd = (sp < 0) ? line : line.substring(0, sp);
  String args = (sp < 0) ? "" : line.substring(sp + 1);
  args.trim();
  cmd.toLowerCase();

  if (cmd == "help")        cmd_help();
  else if (cmd == "clear")  cmd_clear();
  else if (cmd == "info")   cmd_info();
  else if (cmd == "echo")   cmd_echo(args);
  else if (cmd == "mode")   cmd_mode(args);
  else if (cmd == "pwd")    cmd_pwd();
  else if (cmd == "cd")     cmd_cd(args);
  else if (cmd == "ls" || cmd == "dir") cmd_ls(args);
  else if (cmd == "cat")    cmd_cat(args);
  else if (cmd == "write")  cmd_write(args);
  else if (cmd == "rm")     cmd_rm(args);
  else if (cmd == "mkdir")  cmd_mkdir(args);
  else displayPrint("Unknown command", COL_ERROR);
}

// ====================== SETUP / LOOP ======================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(COL_BG);
  tft.setTextSize(1);

  if (!LittleFS.begin(true)) {
    displayPrint("LittleFS failed", COL_ERROR);
  } else {
    ensureFolders();
  }

  displayPrint(String(OS_NAME) + " " + OS_VERSION, COL_PROMPT);
  displayPrint("2.8\" TFT ready", COL_OK);
  displayPrompt();

  Serial.println("[EspOS] started");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      String shown = "CORE:";
      if (cwd != "/") {
        String nice = cwd;
        nice.replace("/", "\\");
        shown += nice;
      }
      shown += "\\> " + inputBuffer;

      displayPrint(shown, COL_PROMPT);
      processCommand(inputBuffer);
      inputBuffer = "";
      displayPrompt();
    }
    else if (c == 8 || c == 127) {
      if (inputBuffer.length() > 0) {
        inputBuffer.remove(inputBuffer.length() - 1);
      }
    }
    else {
      if (inputBuffer.length() < 60) {
        inputBuffer += c;
      }
    }
  }
}
