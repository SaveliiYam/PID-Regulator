#include <Arduino.h>
#include <IPidAutotuner.h>
#include <PidAutotuner.h>
#include <PidController.h>

// Демонстрация relay-автотюна.
// Подставьте реальное измерение и подачу выхода на актуатор.

PidController pid(1.0f, 0.0f, 0.0f);
PidAutotuner tuner(pid);

unsigned long lastMs = 0;
bool tuningDone = false;

void setup() {
  Serial.begin(115200);

  pid.setOutputLimits(0.0f, 255.0f);

  IPidAutotuner& autotune = tuner;
  autotune.setTarget(100.0f);          // целевое значение
  autotune.setOutputStep(40.0f);       // амплитуда реле (±40 вокруг центра)
  autotune.setNoiseBand(1.0f);         // гистерезис / шум
  autotune.setControlRule(IPidAutotuner::Rule::ClassicPid);
  autotune.setOutputLimits(0.0f, 255.0f);
  autotune.setTimeout(90.0f);

  const float measurement = 0.0f;   // текущее значение датчика
  const float outputCenter = 128.0f;
  autotune.start(measurement, outputCenter);

  lastMs = millis();
  Serial.println("Autotune started");
}

void loop() {
  const unsigned long now = millis();
  const float dt = (now - lastMs) / 1000.0f;
  if (dt < 0.01f) {
    return;
  }
  lastMs = now;

  const float measurement = 0.0f;  // TODO: sensor
  IPidAutotuner& autotune = tuner;

  if (autotune.isRunning()) {
    const float output = autotune.update(measurement, dt);
    // TODO: apply output to actuator, e.g. analogWrite(PIN, (int)output);
    Serial.printf("tuning out=%.1f meas=%.1f\n", output, measurement);
    return;
  }

  if (autotune.isFinished() && !tuningDone) {
    tuningDone = true;
    autotune.applyTunings();
    Serial.printf("Done: Kp=%.4f Ki=%.4f Kd=%.4f (Ku=%.4f Pu=%.4f)\n",
                  autotune.getKp(), autotune.getKi(), autotune.getKd(),
                  autotune.getKu(), autotune.getPu());
  }

  if (autotune.isFailed()) {
    Serial.println("Autotune failed (timeout or no oscillation)");
    delay(1000);
    return;
  }

  // Дальше — обычный ПИД с найденными коэффициентами
  const float output = pid.compute(100.0f, measurement, dt);
  Serial.printf("pid out=%.2f\n", output);
}
