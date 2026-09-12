# PIDReg

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Лёгкая библиотека **ПИД-регулятора** для Arduino и PlatformIO.  
Есть абстрактный интерфейс (`IPidController`), реализация (`PidController`) с anti-windup и **автотюнинг** (`IPidAutotuner` / `PidAutotuner`), который записывает найденные коэффициенты в регулятор.

[English version](README.md)

---

## Возможности

- Классический ПИД: \(u = K_p e + K_i \int e\,dt + K_d \frac{de}{dt}\)
- Настраиваемые пределы выхода
- Anti-windup интегратора (ограничение I-составляющей границами выхода)
- Автотюн методом реле (Åström–Hägglund) и правила Ziegler–Nichols
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
#include <PidAutotuner.h>
#include <IPidAutotuner.h>
```

### Arduino IDE

1. Скачайте репозиторий как ZIP (**Code → Download ZIP**) или сделайте `git clone`.
2. В Arduino IDE: **Скетч → Подключить библиотеку → Добавить .ZIP-библиотеку…** и выберите ZIP  
   *(либо скопируйте папку в `Documents/Arduino/libraries/PIDReg`)*.
3. При необходимости перезапустите IDE.
4. Пример: **Файл → Примеры → PIDReg → Basic** или **Autotune**.

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

## Автотюнинг

`PidAutotuner` подаёт на объект релейное воздействие (±`outputStep` вокруг рабочей точки), измеряет амплитуду \(A\) и период \(P_u\) колебаний, считает:

\[
K_u = \frac{4d}{\pi A}
\]

и по выбранному правилу (классический Ziegler–Nichols, Pessen, с/без перерегулирования, только PI) получает `Kp/Ki/Kd`. После успеха вызовите `applyTunings()`, чтобы записать коэффициенты в привязанный `IPidController`.

```cpp
#include <IPidAutotuner.h>
#include <PidAutotuner.h>
#include <PidController.h>

PidController pid;
PidAutotuner tuner(pid);
IPidAutotuner& autotune = tuner;

void setup() {
  pid.setOutputLimits(0, 255);
  autotune.setTarget(100.0f);
  autotune.setOutputStep(40.0f);
  autotune.setNoiseBand(1.0f);
  autotune.setControlRule(IPidAutotuner::Rule::ClassicPid);
  autotune.setOutputLimits(0, 255);
  autotune.start(/*measurement*/ 0.0f, /*outputCenter*/ 128.0f);
}

void loop() {
  float dt = /* ... */;
  float measurement = /* датчик */;

  if (autotune.isRunning()) {
    float out = autotune.update(measurement, dt);
    // подать `out` на исполнительный орган
    return;
  }

  if (autotune.isFinished()) {
    autotune.applyTunings();  // в pid записаны новые Kp/Ki/Kd
  }

  float out = pid.compute(100.0f, measurement, dt);
}
```

| Правило | Когда использовать |
|---------|-------------------|
| `ClassicPid` | Классический Ziegler–Nichols PID |
| `PessenIntegral` | Более агрессивный интеграл |
| `SomeOvershoot` | Более мягкий отклик |
| `NoOvershoot` | Консервативно / малое перерегулирование |
| `PiOnly` | Без D-составляющей |

Полный скетч: [`examples/Autotune/Autotune.ino`](examples/Autotune/Autotune.ino).

**Безопасность:** автотюн намеренно раскачивает процесс. Выбирайте безопасный `outputStep`, задайте лимиты и таймаут, запускайте только на объекте, который это выдержит.

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

### `IPidAutotuner` / `PidAutotuner`

`IPidAutotuner` — интерфейс; `PidAutotuner` — реализация методом реле.

| Метод | Описание |
|--------|----------|
| `start(measurement, outputCenter)` | Старт релейного теста |
| `update(measurement, dt)` | Шаг; возвращает выход на актуатор во время тюнинга |
| `applyTunings()` | Записать найденные коэффициенты в привязанный ПИД |
| `setTarget` / `setOutputStep` / `setNoiseBand` | Параметры автотюна |
| `setControlRule(Rule)` | Набор правил Ziegler–Nichols |
| `setTimeout(seconds)` | Прервать, если нет результата (0 = без лимита) |
| `isRunning` / `isFinished` / `isFailed` | Состояние |
| `getKp/Ki/Kd`, `getKu`, `getPu` | Результаты после успеха |

---

## Советы по настройке

1. Начните с `Ki = 0`, `Kd = 0`, поднимайте `Kp`, пока реакция не станет достаточно быстрой.
2. Добавьте `Ki`, чтобы убрать статическую ошибку; следите за раскачкой и windup.
3. Небольшой `Kd` гасит перерегулирование (шумные датчики лучше сначала отфильтровать).
4. Передавайте стабильный `dt` (лучше фиксированный период регулирования).
5. Задайте `setOutputLimits` под ваш исполнительный орган (ШИМ, скважность, напряжение и т.д.).

TODO: Написать авто-тюн механизм составляющих ПИД.

---

## Структура проекта

```
PID-Regulator/
├── src/
│   ├── IPidController.h
│   ├── PidController.h / .cpp
│   ├── PidAutotuner.h / .cpp
│   └── IPidAutotuner.h
├── examples/Basic/
├── examples/Autotune/
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
