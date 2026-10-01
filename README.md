# EspOS

**Минималистичная модульная операционная система для ESP32**

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

## Возможности (Этап 1)

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

## Структура (модульная версия)

```
EspOS/
├── ArduinoDroid/
│   └── EspOS.ino              ← Для ArduinoDroid
├── EspOS.ino                  ← Точка входа модульной версии
├── config/Config.h
├── kernel/
├── drivers/
├── services/
└── README.md
```

---

## Планы развития

- Этап 2: LittleFS, события, логирование
- Этап 3: Плагины, датчики, связь с Arduino
- Этап 4: WiFi-стек, defensive/offensive инструменты

---

*EspOS — учимся строить ОС с нуля на реальном железе*
