# PIDReg

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Lightweight **PID controller** library for Arduino and PlatformIO.  
Includes an abstract interface (`IPidController`) and a ready-to-use implementation (`PidController`) with output limits and integral anti-windup.

[Русская версия](README.ru.md)

---

## Features

- Classic PID: \(u = K_p e + K_i \int e\,dt + K_d \frac{de}{dt}\)
- Configurable output limits
- Integral anti-windup (clamps the I-term to output bounds)
- Interface-based design — easy to mock or swap implementations
- No Arduino dependency in the core algorithm (`dt` is passed explicitly)
- Works with any PlatformIO / Arduino platform (`architectures=*`)

---

## Installation

### PlatformIO (recommended)

Add the library to your project's `platformio.ini`:

```ini
[env:your_env]
lib_deps =
    https://github.com/SaveliiYam/PID-Regulator.git
```

Pin a version / branch / commit if needed:

```ini
lib_deps =
    https://github.com/SaveliiYam/PID-Regulator.git#v1.0.0
    ; or: https://github.com/SaveliiYam/PID-Regulator.git#main
```

**Local development** (library folder next to your project):

```ini
lib_deps =
    symlink://../PID-Regulator
    ; or: file://../PID-Regulator
```

Then include headers as usual:

```cpp
#include <PidController.h>
#include <IPidController.h>
```

### Arduino IDE

1. Download this repository as a ZIP (**Code → Download ZIP**), or clone it.
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library…** and select the ZIP  
   *(or copy the folder into `Documents/Arduino/libraries/PIDReg`)*.
3. Restart the IDE if needed.
4. Open an example: **File → Examples → PIDReg → Basic**.

---

## Quick start

```cpp
#include <Arduino.h>
#include <PidController.h>

PidController pid(2.0f, 0.5f, 0.1f);  // Kp, Ki, Kd
unsigned long lastMs = 0;

void setup() {
  pid.setOutputLimits(0.0f, 255.0f);  // e.g. PWM range
  lastMs = millis();
}

void loop() {
  const unsigned long now = millis();
  const float dt = (now - lastMs) / 1000.0f;  // seconds
  if (dt < 0.01f) {
    return;  // ~100 Hz
  }
  lastMs = now;

  const float setpoint = 100.0f;
  const float measurement = /* read sensor */ 0.0f;

  const float output = pid.compute(setpoint, measurement, dt);
  // analogWrite(PIN, (int)output);
}
```

Using the interface (useful for tests or alternative implementations):

```cpp
IPidController& regulator = pid;
float output = regulator.compute(setpoint, measurement, dt);
```

See also [`examples/Basic/Basic.ino`](examples/Basic/Basic.ino).

---

## API overview

### `IPidController` (interface)

| Method | Description |
|--------|-------------|
| `compute(setpoint, measurement, dt)` | Compute control output; `dt` in **seconds** |
| `setTunings(kp, ki, kd)` | Update PID gains |
| `setOutputLimits(min, max)` | Enable and set output clamp |
| `reset()` | Clear integral / last error / last output |
| `getKp()`, `getKi()`, `getKd()` | Current gains |
| `getLastOutput()`, `getLastError()` | Last computed values |

### `PidController`

Concrete class implementing `IPidController`.  
Extra: `getIntegral()` — current integral term.

**Constructor:** `PidController(float kp = 1.0f, float ki = 0.0f, float kd = 0.0f)`

---

## Tuning tips

1. Start with `Ki = 0`, `Kd = 0`, increase `Kp` until the response is reasonably fast (some overshoot is OK).
2. Add `Ki` to remove steady-state error; watch for windup / oscillation.
3. Add a small `Kd` to damp overshoot (noisy sensors may need filtering first).
4. Always pass a stable `dt` (fixed control period is best).
5. Set `setOutputLimits` to match your actuator (PWM, duty cycle, voltage, etc.).

---

## Project layout

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

## License

MIT © [Savelii Yam](https://github.com/SaveliiYam)
