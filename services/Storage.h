#ifndef ESPOS_STORAGE_H
#define ESPOS_STORAGE_H

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include "../drivers/Display.h"

class StorageManager {
public:
  StorageManager();

  bool begin();
  void ensureFolders();

  String normalizePath(String path);
  String resolvePath(const String& inputPath, const String& cwd);

  bool isDirectory(const String& path);
  bool exists(const String& path);

  void listDirectory(const String& path, DisplayManager& display);
  bool readFile(const String& path, DisplayManager& display);
  bool writeFile(const String& path, const String& content);
  bool removeFile(const String& path);
  bool makeDirectory(const String& path);

  size_t totalBytes() { return LittleFS.totalBytes(); }
  size_t usedBytes() { return LittleFS.usedBytes(); }
};

#endif // ESPOS_STORAGE_H
