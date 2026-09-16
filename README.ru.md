# PIDReg

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Лёгкая библиотека **ПИД-регулятора** для Arduino и PlatformIO.  
Есть абстрактный интерфейс (`IPidController`), реализация (`PidController`) с anti-windup и **автотюнинг** (`IPidAutotuner` / `PidAutotuner`), который записывает найденные коэффициенты в регулятор.

[English version](README.md) · [Архитектура и AI/RAG-контекст](docs/AI_CONTEXT.md)

---

## Возможности

- Классический ПИД: \(u = K_p e + K_i \int e\,dt + K_d \frac{de}{dt}\)
- Настраиваемые пределы выхода
- Anti-windup интегратора (ограничение I-составляющей границами выхода)
- Автотюн методом реле (Åström–Hägglund) и правила Ziegler–Nichols
- Работа через интерфейс — удобно подменять реализацию и писать тесты
- Ядро алгоритма не зависит от Arduino (`dt` передаётся явно)
- Ядро не зависит от Arduino и объявляет `architectures=*`; локальная
  конфигурация сборки и вывод примеров сейчас ориентированы на ESP32

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
    https://github.com/SaveliiYam/PID-Regulator.git#v1.2.0
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

### Проверка в Wokwi

В репозитории есть готовый локальный сценарий Wokwi. Он запускает relay-autotune
на программной модели инерционного теплового объекта, а затем переключается на
ПИД с найденными коэффициентами. Внешние компоненты не нужны.

1. Установите расширение **Wokwi Simulator** в VS Code/Cursor.
2. Клонируйте репозиторий и откройте его корневую папку.
3. Соберите прошивку:

   ```bash
   pio run -e wokwi
   ```

4. Откройте `diagram.json`, затем выполните **Wokwi: Start Simulator** через
   палитру команд.
5. Следите за выводом Serial Monitor. Сначала появится CSV:

   ```text
   time,phase,setpoint,measurement,error,output,actuator,kp,ki,kd
   ```

   Через несколько моделируемых циклов появится `# AUTOTUNE -> PID`, а
   `phase` изменится с `AUTOTUNE` на `PID`. Измерение должно сойтись к заданию
   `100`. Строки с префиксом `#` показывают переключения реле, периодическое
   состояние автотюна/ПИД, рассчитанные коэффициенты и смену фаз.

Связанные файлы:

- [`examples/Wokwi/Wokwi.ino`](examples/Wokwi/Wokwi.ino) — модель объекта и сценарий
- [`diagram.json`](diagram.json) — схема ESP32
- [`wokwi.toml`](wokwi.toml) — пути к прошивке
- [`platformio.ini`](platformio.ini) — окружение сборки `wokwi`

Если терминал пустой, остановите симуляцию, выполните `pio run -e wokwi` и
запустите её снова из корня репозитория. Если открыть только `diagram.json` на
wokwi.com, локальная прошивка PlatformIO туда не загрузится.

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
  float dt = 0.01f;       // замените измеренным периодом цикла, секунды
  float measurement = 0; // замените показанием датчика

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
| `setTunings(kp, ki, kd)` / `setTunings(PidTunings)` | Обновить коэффициенты |
| `setOutputLimits(min, max)` | Включить и задать ограничение выхода |
| `reset()` | Сбросить интеграл / последнюю ошибку / последний выход |
| `getTunings()` | Получить все коэффициенты как `PidTunings` |
| `getKp()`, `getKi()`, `getKd()` | Текущие коэффициенты |
| `getLastOutput()`, `getLastError()` | Последние вычисленные значения |

### `PidController`

Класс, реализующий `IPidController`.  
Дополнительно: `getIntegral()` — текущая интегральная составляющая.

**Конструкторы:**

- `PidController(float kp = 1.0f, float ki = 0.0f, float kd = 0.0f)`
- `PidController(const PidTunings& tunings)`

### `IPidAutotuner` / `PidAutotuner`

`IPidAutotuner` — интерфейс; `PidAutotuner` — реализация методом реле.

| Метод | Описание |
|--------|----------|
| `start(measurement, outputCenter)` | Старт релейного теста |
| `update(measurement, dt)` | Шаг; возвращает выход на актуатор во время тюнинга |
| `applyTunings()` | Записать найденные коэффициенты в привязанный ПИД |
| `setTarget` / `setOutputStep` / `setNoiseBand` | Параметры автотюна |
| `setOutputLimits(min, max)` | Ограничить релейный выход |
| `setControlRule(Rule)` | Набор правил Ziegler–Nichols |
| `setTuningRule(IPidTuningRule)` | Подключить свою стратегию преобразования Ku/Pu |
| `setIgnoreCycles` / `setSettleCycles` | Настроить пропуск переходного процесса и усреднение |
| `setTimeout(seconds)` | Прервать, если нет результата (0 = без лимита) |
| `cancel()` / `getState()` | Остановить тюнинг / получить полное состояние |
| `isRunning` / `isFinished` / `isFailed` | Проверки состояния |
| `getTunings`, `getKp/Ki/Kd`, `getKu`, `getPu` | Результаты после успеха |
| `getLastOutput()` | Последний релейный выход |

### Своя стратегия настройки

Реализуйте `IPidTuningRule`, чтобы добавить формулу без изменения тюнера:

```cpp
#include <IPidTuningRule.h>

class ConservativeRule : public IPidTuningRule {
public:
  PidTunings compute(float ku, float pu) const override {
    (void)pu;
    return PidTunings(0.15f * ku, 0.0f, 0.0f);
  }
};

static ConservativeRule rule;  // должна жить дольше тюнера
tuner.setTuningRule(rule);
```

## Особенности поведения

- `dt` всегда задаётся в секундах.
- При некорректном `dt` (`<= 0` или не конечное число) возвращается предыдущий
  выход без обновления состояния регулятора или тюнера.
- Задание, измерение, коэффициенты и границы не проверяются на конечность:
  вызывающий код обязан передавать конечные значения.
- Ограничения выключены до вызова `setOutputLimits()`; перепутанные границы
  меняются местами автоматически. У `PidController` установка границ также
  сразу ограничивает текущий интеграл и последний выход.
- `setTunings()` меняет коэффициенты, но сохраняет накопленное состояние;
  при необходимости отдельно вызовите `reset()`.
- `setOutputStep()` и `setNoiseBand()` используют абсолютные значения.
- Значения автотюна по умолчанию: классический PID, шаг `50`, зона шума `1`,
  два пропущенных и шесть усредняемых полупериодов, таймаут 60 секунд.
- `start()` разрешён из любого состояния и сбрасывает статистику и прошлые
  результаты. `cancel()` меняет состояние только из `Running`.
- `update()` вне `Running` возвращает последний релейный выход.
- `setControlRule()` и `setTuningRule()` выбирают одну и ту же стратегию;
  действует последний вызванный метод.
- `applyTunings()` работает только после успешного тюнинга и сбрасывает ПИД.
- `cancel()`, `Finished` и `Failed` не устанавливают безопасный выход актуатора:
  приложение должно сразу выбрать следующий выход (ПИД, центр или ноль).
- Объект пользовательского `IPidTuningRule` должен жить дольше тюнера.
- Сохранение коэффициентов в EEPROM/NVS выполняет приложение.
- Классы хранят состояние и не являются потокобезопасными.

### Вспомогательные публичные типы

| Тип | Назначение |
|-----|------------|
| `PidTunings` | Объект значений с публичными полями `kp`, `ki`, `kd` |
| `PidAutotuneState` | `Idle`, `Running`, `Finished`, `Failed` |
| `PidAutotuneRule` | Идентификаторы пяти встроенных правил |
| `IPidTuningRule` | Интерфейс стратегии `Ku/Pu -> PidTunings` |
| `ZieglerNicholsRule` | Встроенная стратегия; `forRule()` возвращает статическое правило |
| `OutputClamp` | Общий ограничитель; обычно приложению напрямую не нужен |

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
│   ├── PidController.h / .cpp
│   ├── PidTunings.h
│   ├── OutputClamp.h
│   ├── IPidTuningRule.h
│   ├── ZieglerNicholsRule.h / .cpp
│   ├── PidAutotuneTypes.h
│   ├── PidAutotuner.h / .cpp
│   └── IPidAutotuner.h
├── examples/Basic/
├── examples/Autotune/
├── examples/Wokwi/
├── docs/AI_CONTEXT.md
├── diagram.json
├── wokwi.toml
├── platformio.ini
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
