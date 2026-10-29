# EspOS Roadmap

## Текущая версия

**v0.3.0-alpha** — Переход на 2.8" TFT 240×320

- Новый основной дисплей: 2.8" ILI9341 TFT
- Цветной текстовый Shell
- Подготовка к SD-карте (слот на плате дисплея)

---

## Stage 1 — Foundation ✅

- [x] Kernel, Display Manager, Shell, Command system
- [x] ArduinoDroid single-file support

**Released:** `v0.1.0-alpha`

---

## Stage 2 — Storage & System Core ✅ (частично)

- [x] LittleFS as `CORE:`
- [x] File commands + current directory (`cd`, `pwd`)
- [ ] SD Card support (`SD:`) — следующий шаг
- [ ] Simple config system
- [ ] Event Bus

---

## Stage 3 — New Display & UI  ← сейчас здесь

- [x] Переход на 2.8" TFT 240×320 (ILI9341)
- [ ] Базовая работа с SD-слотом на плате дисплея
- [ ] Улучшенный Status-экран
- [ ] Простой файловый менеджер (графический)

**Target:** `v0.3.0`

---

## Stage 4 — Extensibility

- [ ] Plugin / Module system
- [ ] Buttons + Touch
- [ ] Связь с Arduino (Uno/Nano/Mini)
- [ ] Sensor drivers

---

## Stage 5 — Interpreters & Runtime (CraftOS-inspired)

- [ ] Lua interpreter
- [ ] Python (MicroPython / упрощённый)
- [ ] Возможность запуска пользовательских скриптов с SD/CORE
- [ ] Простая система пакетов / программ

---

## Future ideas

- Поддержка русского языка (кириллица)
- Большие параллельные LCD (KS0108)
- RFID
- WiFi tools
- Mesh

---

*Last updated: 2026-10-29*
