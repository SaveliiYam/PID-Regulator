#include "PidController.h"

#include <math.h>

PidController::PidController(float kp, float ki, float kd)
    : _tunings(kp, ki, kd),
      _integral(0.0f),
      _lastError(0.0f),
      _lastOutput(0.0f) {}

PidController::PidController(const PidTunings& tunings)
    : PidController(tunings.kp, tunings.ki, tunings.kd) {}

float PidController::compute(float setpoint, float measurement, float dt) {
  if (dt <= 0.0f || !isfinite(dt)) {
    return _lastOutput;
  }

  const float error = setpoint - measurement;
  const float pTerm = _tunings.kp * error;

  _integral += _tunings.ki * error * dt;
  if (_clamp.isEnabled()) {
    // Anti-windup: не даём интегралу уводить выход за пределы
    if (_integral > _clamp.max()) {
      _integral = _clamp.max();
    } else if (_integral < _clamp.min()) {
      _integral = _clamp.min();
    }
  }

  const float dTerm = _tunings.kd * (error - _lastError) / dt;

  float output = pTerm + _integral + dTerm;
  output = _clamp.apply(output);

  _lastError = error;
  _lastOutput = output;
  return output;
}

void PidController::setTunings(float kp, float ki, float kd) {
  _tunings.kp = kp;
  _tunings.ki = ki;
  _tunings.kd = kd;
}

void PidController::setTunings(const PidTunings& tunings) {
  _tunings = tunings;
}

void PidController::setOutputLimits(float min, float max) {
  _clamp.setLimits(min, max);
  _integral = _clamp.apply(_integral);
  _lastOutput = _clamp.apply(_lastOutput);
}

void PidController::reset() {
  _integral = 0.0f;
  _lastError = 0.0f;
  _lastOutput = 0.0f;
}
