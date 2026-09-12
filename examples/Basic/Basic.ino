#include <Arduino.h>
#include <IPidController.h>
#include <PidController.h>

PidController pid(2.0f, 0.5f, 0.1f);

unsigned long lastMs = 0;

void setup() {
  Serial.begin(115200);
  pid.setOutputLimits(0.0f, 255.0f);
  lastMs = millis();
}

void loop() {
  const unsigned long now = millis();
  const float dt = (now - lastMs) / 1000.0f;
  if (dt < 0.01f) {
    return;
  }
  lastMs = now;

  const float setpoint = 100.0f;
  const float measurement = 0.0f;  // replace with sensor reading

  IPidController& regulator = pid;
  const float output = regulator.compute(setpoint, measurement, dt);

  Serial.printf("out=%.2f err=%.2f\n", output, regulator.getLastError());
}
