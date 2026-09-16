# PIDReg

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Lightweight **PID controller** library for Arduino and PlatformIO.  
Includes an abstract interface (`IPidController`), a ready-to-use implementation (`PidController`) with anti-windup, and **relay autotune** (`IPidAutotuner` / `PidAutotuner`) that writes gains into the controller.

[Русская версия](README.ru.md) · [Architecture and AI/RAG context](docs/AI_CONTEXT.md)

---

## Features

- Classic PID: \(u = K_p e + K_i \int e\,dt + K_d \frac{de}{dt}\)
- Configurable output limits
- Integral anti-windup (clamps the I-term to output bounds)
- Relay autotune (Åström–Hägglund) with Ziegler–Nichols-style rules
- Interface-based design — easy to mock or swap implementations
- No Arduino dependency in the core algorithm (`dt` is passed explicitly)
- Core code is Arduino-independent and declares `architectures=*`; the bundled
  local build configuration and serial examples are currently ESP32-oriented

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
    https://github.com/SaveliiYam/PID-Regulator.git#v1.2.0
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
#include <PidAutotuner.h>
#include <IPidAutotuner.h>
```

### Arduino IDE

1. Download this repository as a ZIP (**Code → Download ZIP**), or clone it.
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library…** and select the ZIP  
   *(or copy the folder into `Documents/Arduino/libraries/PIDReg`)*.
3. Restart the IDE if needed.
4. Open an example: **File → Examples → PIDReg → Basic** or **Autotune**.

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

## Autotune

`PidAutotuner` drives the plant with a relay (±`outputStep` around a working point), measures the oscillation amplitude \(A\) and period \(P_u\), then computes:

\[
K_u = \frac{4d}{\pi A}
\]

and applies a selected rule (classic Ziegler–Nichols, Pessen, some/no overshoot, PI-only). When finished, call `applyTunings()` to write `Kp/Ki/Kd` into the bound `IPidController`.

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
  float dt = 0.01f;       // replace with the measured loop period, seconds
  float measurement = 0; // replace with a sensor reading

  if (autotune.isRunning()) {
    float out = autotune.update(measurement, dt);
    // write `out` to the actuator
    return;
  }

  if (autotune.isFinished()) {
    autotune.applyTunings();  // pid now has new Kp/Ki/Kd
  }

  float out = pid.compute(100.0f, measurement, dt);
}
```

| Rule | Use when |
|------|----------|
| `ClassicPid` | Default Ziegler–Nichols PID |
| `PessenIntegral` | Faster integral action |
| `SomeOvershoot` | Milder response |
| `NoOvershoot` | Conservative / little overshoot |
| `PiOnly` | No D term |

Full sketch: [`examples/Autotune/Autotune.ino`](examples/Autotune/Autotune.ino).

**Safety:** autotune deliberately oscillates the process. Use a safe `outputStep`, set limits/timeout, and only run on a plant that can tolerate it.

---

## API overview

### `IPidController` (interface)

| Method | Description |
|--------|-------------|
| `compute(setpoint, measurement, dt)` | Compute control output; `dt` in **seconds** |
| `setTunings(kp, ki, kd)` / `setTunings(PidTunings)` | Update PID gains |
| `setOutputLimits(min, max)` | Enable and set output clamp |
| `reset()` | Clear integral / last error / last output |
| `getTunings()` | Return all gains as `PidTunings` |
| `getKp()`, `getKi()`, `getKd()` | Current gains |
| `getLastOutput()`, `getLastError()` | Last computed values |

### `PidController`

Concrete class implementing `IPidController`.  
Extra: `getIntegral()` — current integral term.

**Constructors:**

- `PidController(float kp = 1.0f, float ki = 0.0f, float kd = 0.0f)`
- `PidController(const PidTunings& tunings)`

### `IPidAutotuner` / `PidAutotuner`

`IPidAutotuner` is the interface; `PidAutotuner` is the relay implementation.

| Method | Description |
|--------|-------------|
| `start(measurement, outputCenter)` | Begin relay test |
| `update(measurement, dt)` | Step; returns actuator output while tuning |
| `applyTunings()` | Write found gains into the bound PID |
| `setTarget` / `setOutputStep` / `setNoiseBand` | Autotune parameters |
| `setOutputLimits(min, max)` | Limit relay output |
| `setControlRule(Rule)` | Ziegler–Nichols-style rule set |
| `setTuningRule(IPidTuningRule)` | Inject a custom Ku/Pu conversion strategy |
| `setIgnoreCycles` / `setSettleCycles` | Configure transient rejection and averaging |
| `setTimeout(seconds)` | Abort if no result (0 = no limit) |
| `cancel()` / `getState()` | Stop tuning / read full state |
| `isRunning` / `isFinished` / `isFailed` | State helpers |
| `getTunings`, `getKp/Ki/Kd`, `getKu`, `getPu` | Results after success |
| `getLastOutput()` | Last relay output |

### Custom tuning strategy

Implement `IPidTuningRule` to add a formula without modifying the autotuner:

```cpp
#include <IPidTuningRule.h>

class ConservativeRule : public IPidTuningRule {
public:
  PidTunings compute(float ku, float pu) const override {
    (void)pu;
    return PidTunings(0.15f * ku, 0.0f, 0.0f);
  }
};

static ConservativeRule rule;  // must outlive the tuner
tuner.setTuningRule(rule);
```

## Behavioral notes

- `dt` is always measured in seconds.
- Invalid `dt` (`<= 0` or non-finite) returns the previous output without
  updating controller/autotuner state.
- Setpoints, measurements, gains, and output limits are not validated for
  finiteness; callers must provide finite values.
- Output limits are disabled until `setOutputLimits()` is called; reversed
  limits are swapped automatically. On `PidController`, setting limits also
  clamps the current integral term and last output immediately.
- `setTunings()` changes gains but preserves the accumulated controller state;
  call `reset()` explicitly when that is not desired.
- `setOutputStep()` and `setNoiseBand()` use absolute values.
- Autotune defaults: classic PID rule, step `50`, noise band `1`, two ignored
  half-cycles, six averaged half-cycles, and a 60-second timeout.
- `start()` is allowed from any state and resets prior experiment statistics
  and results. `cancel()` changes state only while `Running`.
- `update()` outside `Running` returns the last relay output.
- `setControlRule()` and `setTuningRule()` replace the same active strategy;
  the most recent call wins.
- `applyTunings()` works only after successful tuning and resets the PID state.
- `cancel()`, `Finished`, and `Failed` do not force a safe actuator value; the
  application must immediately choose the next output (PID, center, or zero).
- A custom `IPidTuningRule` object must outlive the tuner that references it.
- Gain persistence (EEPROM/NVS) is the application's responsibility.
- The stateful classes are not thread-safe.

### Supporting public types

| Type | Purpose |
|------|---------|
| `PidTunings` | Value object with public `kp`, `ki`, and `kd` fields |
| `PidAutotuneState` | `Idle`, `Running`, `Finished`, `Failed` |
| `PidAutotuneRule` | Identifiers for the five built-in rules |
| `IPidTuningRule` | Custom `Ku/Pu -> PidTunings` strategy interface |
| `ZieglerNicholsRule` | Built-in strategy; `forRule()` returns a static rule |
| `OutputClamp` | Shared limiting helper; normally not needed by applications |

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
├── docs/AI_CONTEXT.md
├── platformio.ini
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
