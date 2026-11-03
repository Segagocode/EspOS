#include "Storage.h"

StorageManager::StorageManager() {
}

bool StorageManager::begin() {
  if (!LittleFS.begin(true)) {
    return false;
  }
  ensureFolders();
  return true;
}

void StorageManager::ensureFolders() {
  if (!LittleFS.exists("/system")) LittleFS.mkdir("/system");
  if (!LittleFS.exists("/config")) LittleFS.mkdir("/config");
  if (!LittleFS.exists("/logs"))   LittleFS.mkdir("/logs");
}

String StorageManager::normalizePath(String path) {
  if (path.length() == 0) return "/";
  if (!path.startsWith("/")) path = "/" + path;
  while (path.indexOf("//") >= 0) path.replace("//", "/");
  if (path.length() > 1 && path.endsWith("/")) path.remove(path.length() - 1);
  return path.length() == 0 ? "/" : path;
}

String StorageManager::resolvePath(const String& inputPath, const String& cwd) {
  String full = inputPath.startsWith("/") ? inputPath : (cwd + "/" + inputPath);
  String segments[16];
  int segCount = 0;

  int start = 0;
  while (start < (int)full.length()) {
    int slash = full.indexOf('/', start);
    if (slash == -1) slash = full.length();
    String token = full.substring(start, slash);
    start = slash + 1;

    if (token.length() == 0 || token == ".") continue;
    if (token == "..") {
      if (segCount > 0) segCount--;
    } else if (segCount < 16) {
      segments[segCount++] = token;
    }
  }

  if (segCount == 0) return "/";
  String result = "";
  for (int i = 0; i < segCount; i++) {
    result += "/" + segments[i];
  }
  return result;
}

bool StorageManager::isDirectory(const String& path) {
  File f = LittleFS.open(path.c_str());
  if (!f) return false;
  bool isDir = f.isDirectory();
  f.close();
  return isDir;
}

bool StorageManager::exists(const String& path) {
  return LittleFS.exists(path.c_str());
}

void StorageManager::listDirectory(const String& path, DisplayManager& display) {
  File root = LittleFS.open(path.c_str());
  if (!root || !root.isDirectory()) {
    display.print("Not a directory", COL_ERROR);
    return;
  }

  File f = root.openNextFile();
  if (!f) {
    display.print("(empty)", COL_TEXT);
    root.close();
    return;
  }

  while (f) {
    String name = f.name();
    int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    if (f.isDirectory()) {
      display.print("[DIR] " + name, COL_DIR);
    } else {
      display.print(name + " " + String(f.size()) + "b", COL_TEXT);
    }
    f = root.openNextFile();
  }
  root.close();
}

bool StorageManager::readFile(const String& path, DisplayManager& display) {
  File f = LittleFS.open(path.c_str(), "r");
  if (!f || f.isDirectory()) {
    return false;
  }
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      display.print(line, COL_TEXT);
    }
  }
  f.close();
  return true;
}

bool StorageManager::writeFile(const String& path, const String& content) {
  File f = LittleFS.open(path.c_str(), "w");
  if (!f) return false;
  f.println(content);
  f.close();
  return true;
}

bool StorageManager::removeFile(const String& path) {
  return LittleFS.remove(path.c_str());
}

bool StorageManager::makeDirectory(const String& path) {
  return LittleFS.mkdir(path.c_str());
}
