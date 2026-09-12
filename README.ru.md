# PIDReg

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Лёгкая библиотека **ПИД-регулятора** для Arduino и PlatformIO.  
Есть абстрактный интерфейс (`IPidController`) и готовая реализация (`PidController`) с ограничением выхода и anti-windup по интегральной составляющей.

[English version](README.md)

---

## Возможности

- Классический ПИД: \(u = K_p e + K_i \int e\,dt + K_d \frac{de}{dt}\)
- Настраиваемые пределы выхода
- Anti-windup интегратора (ограничение I-составляющей границами выхода)
- Работа через интерфейс — удобно подменять реализацию и писать тесты
- Ядро алгоритма не зависит от Arduino (`dt` передаётся явно)
- Подходит для любой платформы PlatformIO / Arduino (`architectures=*`)

---

## Установка

### PlatformIO (рекомендуется)

В `platformio.ini` вашего проекта:

```ini
[env:your_env]
lib_deps =
    https://github.com/SaveliiYam/PID-Regulator.git
```

При необходимости закрепите версию / ветку / коммит:

```ini
lib_deps =
    https://github.com/SaveliiYam/PID-Regulator.git#v1.0.0
    ; или: https://github.com/SaveliiYam/PID-Regulator.git#main
```

**Локальная разработка** (папка библиотеки рядом с проектом):

```ini
lib_deps =
    symlink://../PID-Regulator
    ; или: file://../PID-Regulator
```

Подключение заголовков:

```cpp
#include <PidController.h>
#include <IPidController.h>
```

### Arduino IDE

1. Скачайте репозиторий как ZIP (**Code → Download ZIP**) или сделайте `git clone`.
2. В Arduino IDE: **Скетч → Подключить библиотеку → Добавить .ZIP-библиотеку…** и выберите ZIP  
   *(либо скопируйте папку в `Documents/Arduino/libraries/PIDReg`)*.
3. При необходимости перезапустите IDE.
4. Пример: **Файл → Примеры → PIDReg → Basic**.

---

## Быстрый старт

```cpp
#include <Arduino.h>
#include <PidController.h>

PidController pid(2.0f, 0.5f, 0.1f);  // Kp, Ki, Kd
unsigned long lastMs = 0;

void setup() {
  pid.setOutputLimits(0.0f, 255.0f);  // например, диапазон ШИМ
  lastMs = millis();
}

void loop() {
  const unsigned long now = millis();
  const float dt = (now - lastMs) / 1000.0f;  // секунды
  if (dt < 0.01f) {
    return;  // ~100 Гц
  }
  lastMs = now;

  const float setpoint = 100.0f;
  const float measurement = /* показание датчика */ 0.0f;

  const float output = pid.compute(setpoint, measurement, dt);
  // analogWrite(PIN, (int)output);
}
```

Через интерфейс (удобно для тестов и альтернативных реализаций):

```cpp
IPidController& regulator = pid;
float output = regulator.compute(setpoint, measurement, dt);
```

См. также [`examples/Basic/Basic.ino`](examples/Basic/Basic.ino).

---

## Обзор API

### `IPidController` (интерфейс)

| Метод | Описание |
|--------|----------|
| `compute(setpoint, measurement, dt)` | Расчёт управляющего воздействия; `dt` в **секундах** |
| `setTunings(kp, ki, kd)` | Обновить коэффициенты |
| `setOutputLimits(min, max)` | Включить и задать ограничение выхода |
| `reset()` | Сбросить интеграл / последнюю ошибку / последний выход |
| `getKp()`, `getKi()`, `getKd()` | Текущие коэффициенты |
| `getLastOutput()`, `getLastError()` | Последние вычисленные значения |

### `PidController`

Класс, реализующий `IPidController`.  
Дополнительно: `getIntegral()` — текущая интегральная составляющая.

**Конструктор:** `PidController(float kp = 1.0f, float ki = 0.0f, float kd = 0.0f)`

---

## Советы по настройке

1. Начните с `Ki = 0`, `Kd = 0`, поднимайте `Kp`, пока реакция не станет достаточно быстрой.
2. Добавьте `Ki`, чтобы убрать статическую ошибку; следите за раскачкой и windup.
3. Небольшой `Kd` гасит перерегулирование (шумные датчики лучше сначала отфильтровать).
4. Передавайте стабильный `dt` (лучше фиксированный период регулирования).
5. Задайте `setOutputLimits` под ваш исполнительный орган (ШИМ, скважность, напряжение и т.д.).

---

## Структура проекта

```
PID-Regulator/
├── src/
│   ├── IPidController.h
│   ├── PidController.h
│   └── PidController.cpp
├── examples/Basic/
├── library.json
├── library.properties
├── keywords.txt
├── LICENSE
├── README.md
└── README.ru.md
```

---

## Лицензия

MIT © [Savelii Yam](https://github.com/SaveliiYam)
