# EspOS Roadmap

## Текущая версия

**v0.2.1-alpha**

- LittleFS as **CORE:**
- Current directory + `cd` + `pwd`
- Relative paths support
- Dynamic prompt (shows current path)

---

## Stage 1 — Foundation ✅

- [x] Project structure
- [x] Display Manager (SHELL / STATUS)
- [x] Command Registry
- [x] Shell
- [x] Basic Kernel
- [x] ArduinoDroid single-file support

**Released:** `v0.1.0-alpha`

---

## Stage 2 — Storage & System Core  ← сейчас здесь

- [x] LittleFS as system volume (`CORE:`)
- [x] Auto-create `/system`, `/config`, `/logs`
- [x] File commands: `ls`, `cat`, `write`, `rm`, `mkdir`
- [x] Better path handling & current directory (`cd`, `pwd`)
- [ ] Simple config system
- [ ] SD Card support (`SD:`)
- [ ] Event Bus

**Target:** `v0.2.0` → next toward full 0.2.0

---

## Stage 3 — Extensibility

- [ ] Plugin / Module system
- [ ] Buttons support
- [ ] Communication with Arduino boards
- [ ] Sensor drivers
- [ ] Improved multi-screen UI

**Target:** `v0.3.0`

---

## Stage 4 — Networking & Tools

- [ ] WiFi Manager
- [ ] Packet monitoring foundation
- [ ] Defensive tools
- [ ] Lab-mode modules

**Target:** `v0.4.0`

---

## Future ideas

- **Поддержка русского языка** (кириллица в Shell + OLED)
- Larger displays (KS0108 и др.)
- RFID (RC522)
- Simple scripting
- Power management
- Mesh experiments

---

*Last updated: 2026-10-02*
