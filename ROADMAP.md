# EspOS Roadmap

## Current version

**v0.4.0-alpha**

### Highlights
- Visible input line (no more blind typing)
- Keyboard with `.` `/` `-`
- Fixed terminal scrolling
- Proper `resolvePath` with `..` support
- Command table dispatcher
- WiFi connect + save config
- OTA update command with progress bar

---

## Design principle

> Terminal is the core.  
> Graphics is a complement, not a replacement.

---

## Stage status

### Stage 1 — Foundation ✅
- Kernel/Shell/Commands
- ArduinoDroid single-file workflow

### Stage 2 — Storage ✅
- LittleFS as `CORE:`
- Path handling, cd/pwd/ls/cat/write...

### Stage 3 — Display & Input ✅
- 2.8" ILI9341
- On-screen keyboard
- Touch integration

### Stage 4 — Connectivity & Autonomy  ← now
- [x] WiFi command + saved config
- [x] OTA update command
- [ ] Better touch calibration UI
- [ ] Auto-update channel / version check

### Stage 5 — Runtime
- Lua / scripting
- App-like launch from files

---

## New commands in v0.4.0

```
wifi                  # status
wifi <ssid> <pass>    # connect + save
update <url>          # OTA flash .bin from URL
```

---

*Updated: 2026-10-30*
