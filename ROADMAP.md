# EspOS Roadmap

## Текущая версия

**v0.1.0-alpha** — Stage 1 complete

- Kernel
- Display Manager (SHELL / STATUS modes)
- Command system
- Shell (OLED + Serial as keyboard)
- Single-file version for ArduinoDroid

---

## Stage 1 — Foundation ✅ (done)

- [x] Project structure
- [x] Display Manager with multiple modes
- [x] Command Registry
- [x] Shell
- [x] Basic Kernel
- [x] ArduinoDroid single-file support

**Release:** `v0.1.0-alpha`

---

## Stage 2 — Storage & System Core

**Цель:** Появление настоящей файловой системы и внутренней структуры ОС

- [ ] LittleFS (internal flash)
  - `/system/` for configs
  - Basic file commands: `ls`, `cat`, `rm`, `write`
- [ ] SD Card support (SPI)
  - microSD as main large storage (`D:\`)
  - Mount / unmount commands
  - File operations on SD
- [ ] Simple config system (`/system/config.txt`)
- [ ] Event Bus (modules can talk to each other)
- [ ] Better logging

**Planned version:** `v0.2.0`

---

## Stage 3 — Extensibility

- [ ] Plugin / Module system
- [ ] Input Manager (buttons support)
- [ ] Communication with external Arduino (Uno / Nano / Mini) via UART or I2C
- [ ] Basic sensor drivers from starter kit
- [ ] Improved multi-mode UI (easy switching between screens)

**Planned version:** `v0.3.0`

---

## Stage 4 — Networking & Tools

- [ ] WiFi Manager
- [ ] Basic network status screen
- [ ] Packet monitoring foundation
- [ ] Defensive tools (detect deauth, etc.)
- [ ] Lab-mode offensive modules (strictly controlled)

**Planned version:** `v0.4.0`

---

## Future ideas (backlog)

- Larger displays support (KS0108 128x64 parallel, etc.)
- RFID (RC522) integration
- Simple scripting language
- Multi-tasking improvements
- Power management / deep sleep
- Mesh networking experiments (Meshtastic-inspired)
- Graphical UI experiments

---

## Versioning philosophy

We use simple semantic-like versions:

- `0.x.y-alpha` — early development
- Major stage completion → bump minor version (`0.1` → `0.2`)
- Small improvements and fixes → patch (`0.2.0` → `0.2.1`)

Every meaningful milestone gets a version bump so progress is visible and motivating.

---

*Last updated: 2026-10-01*
