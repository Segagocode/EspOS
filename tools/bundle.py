#!/usr/bin/env python3
"""
EspOS Bundle Tool
Merges all modular C++ files (.h / .cpp) into a single monolithic
ArduinoDroid/EspOS.ino file for Android compilation.
"""

import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_FILE = os.path.join(ROOT, "ArduinoDroid", "EspOS.ino")

FILES_IN_ORDER = [
    # Config
    "config/Config.h",
    # Drivers headers & sources
    "drivers/Display.h",
    "drivers/Display.cpp",
    "drivers/Touch.h",
    "drivers/Touch.cpp",
    # Services headers & sources
    "services/Storage.h",
    "services/Storage.cpp",
    "services/NetManager.h",
    "services/NetManager.cpp",
    "services/WirelessTools.h",
    "services/WirelessTools.cpp",
    "services/Shell.h",
    "services/Shell.cpp",
    # Kernel
    "kernel/Kernel.h",
    "kernel/Kernel.cpp",
    # Entry point
    "EspOS.ino"
]

STANDARD_INCLUDES = set([
    "#include <Arduino.h>",
    "#include <SPI.h>",
    "#include <Adafruit_GFX.h>",
    "#include <Adafruit_ILI9341.h>",
    "#include <XPT2046_Touchscreen.h>",
    "#include <FS.h>",
    "#include <LittleFS.h>",
    "#include <WiFi.h>",
    "#include <HTTPClient.h>",
    "#include <HTTPUpdate.h>",
    "#include <WiFiClientSecure.h>",
    '#include "esp_wifi.h"'
])

def bundle():
    print(f"Bundling EspOS into {OUT_FILE}...")
    os.makedirs(os.path.dirname(OUT_FILE), exist_ok=True)

    header = """/*
 * EspOS - Minimal Modular OS for ESP32
 * v0.5.0-marauder: Single-file release for ArduinoDroid
 *
 * Generated automatically by tools/bundle.py
 * DO NOT EDIT DIRECTLY: edit modular files in config/, drivers/, services/, kernel/
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "esp_wifi.h"

"""

    body_lines = []
    
    for rel_path in FILES_IN_ORDER:
        full_path = os.path.join(ROOT, rel_path)
        if not os.path.exists(full_path):
            print(f"Warning: {rel_path} not found!")
            continue

        with open(full_path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        body_lines.append(f"\n// ============================================================\n")
        body_lines.append(f"// FILE: {rel_path}\n")
        body_lines.append(f"// ============================================================\n\n")

        for line in lines:
            trimmed = line.strip()
            # Skip include guards
            if trimmed.startswith("#ifndef ESPOS_") or trimmed.startswith("#define ESPOS_") or trimmed.startswith("#endif // ESPOS_"):
                continue
            # Skip local header includes
            if re.match(r'#include\s+["<](config/|drivers/|services/|kernel/|\.\./)', trimmed):
                continue
            # Skip standard includes already included at top
            if trimmed in STANDARD_INCLUDES:
                continue
            body_lines.append(line)

    with open(OUT_FILE, "w", encoding="utf-8") as out:
        out.write(header)
        out.writelines(body_lines)

    print(f"Successfully generated {OUT_FILE} ({os.path.getsize(OUT_FILE)} bytes)")

if __name__ == "__main__":
    bundle()
