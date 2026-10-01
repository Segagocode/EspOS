# EspOS

**Минималистичная модульная операционная система для ESP32**

Текущая версия: **v0.1.0-alpha**

## Две версии в репозитории

| Папка / файл              | Для кого                        | Описание |
|---------------------------|----------------------------------|----------|
| **ArduinoDroid/EspOS.ino** | ArduinoDroid (Android)          | Один большой файл — просто открыл и прошил |
| Корень репозитория        | Arduino IDE (ПК) / разработка   | Модульная структура (.h/.cpp) |

---

## Быстрый старт (ArduinoDroid)

1. Скачай/клонируй репозиторий
2. Открой папку `ArduinoDroid`
3. Открой файл **EspOS.ino**
4. Убедись, что установлены библиотеки:
   - Adafruit GFX Library
   - Adafruit SSD1306
   - Adafruit BusIO
5. Выбери плату ESP32 и загрузи

---

## Возможности (v0.1.0-alpha)

- Display Manager с несколькими режимами (SHELL / STATUS)
- Система команд (легко добавлять новые)
- Shell: OLED + Serial как клавиатура
- Команды: `help`, `clear`, `dir`, `info`, `echo`, `mode`

### Примеры команд

```
help
info
mode status
mode shell
echo Hello EspOS
clear
```

---

## Roadmap

Подробный план развития смотри в [ROADMAP.md](ROADMAP.md)

Кратко:

- **Stage 2** — LittleFS + SD Card + Event Bus
- **Stage 3** — Plugins, кнопки, связь с Arduino
- **Stage 4** — WiFi tools

---

## Структура

```
EspOS/
├── ArduinoDroid/
│   └── EspOS.ino              ← Для ArduinoDroid
├── EspOS.ino                  ← Точка входа модульной версии
├── config/
├── kernel/
├── drivers/
├── services/
├── ROADMAP.md
└── README.md
```

---

*EspOS — учимся строить ОС с нуля на реальном железе*
