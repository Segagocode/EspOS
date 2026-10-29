# EspOS Roadmap

## Текущая версия

**v0.3.1-alpha**

- 2.8" TFT 240×320 (ILI9341)
- Рабочий цветной Shell
- LittleFS как `CORE:`
- Команды: ls, cd, pwd, cat, write, rm, mkdir, help, info, mode...

---

## Главный принцип дизайна EspOS

> **Терминал — основа.**  
> **Графика — дополнение, а не замена.**

Мы сознательно сохраняем мощный текстовый Shell как главный способ управления системой.  
Графические экраны и элементы добавляются постепенно и только там, где они реально дают удобство, не убивая производительность ESP32.

---

## Stage 1 — Foundation ✅

- [x] Kernel + Shell + Command system
- [x] ArduinoDroid single-file support

---

## Stage 2 — Storage ✅ (почти)

- [x] LittleFS as `CORE:`
- [x] Current directory + path handling
- [ ] SD Card support (`SD:`) ← следующий большой шаг

---

## Stage 3 — Display & Basic UI  ← сейчас

- [x] Переход на 2.8" TFT 240×320
- [x] Цветной текстовый Shell
- [ ] SD-карта через слот на дисплее
- [ ] Улучшенный Status-экран
- [ ] Простой графический файловый менеджер (список)

---

## Stage 4 — Extensibility

- [ ] Buttons + Touch
- [ ] Plugin system
- [ ] Связь с Arduino-платами

---

## Stage 5 — Runtime (CraftOS-inspired)

- [ ] Lua interpreter
- [ ] Возможность запуска скриптов с CORE:/SD:
- [ ] Простая система программ

---

## Future

- Русский язык
- Более сложная графика (осторожно)
- WiFi tools
- Mesh

---

*Last updated: 2026-10-29*
