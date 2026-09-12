#include "PidController.h"

#include <math.h>

PidController::PidController(float kp, float ki, float kd)
    : _kp(kp),
      _ki(ki),
      _kd(kd),
      _integral(0.0f),
      _lastError(0.0f),
      _lastOutput(0.0f),
      _outMin(0.0f),
      _outMax(0.0f),
      _limitsEnabled(false) {}

float PidController::compute(float setpoint, float measurement, float dt) {
  if (dt <= 0.0f || !isfinite(dt)) {
    return _lastOutput;
  }

  const float error = setpoint - measurement;

  const float pTerm = _kp * error;

  _integral += _ki * error * dt;
  if (_limitsEnabled) {
    // Anti-windup: не даём интегралу уводить выход за пределы
    if (_integral > _outMax) {
      _integral = _outMax;
    } else if (_integral < _outMin) {
      _integral = _outMin;
    }
  }

  const float dTerm = _kd * (error - _lastError) / dt;

  float output = pTerm + _integral + dTerm;
  output = clamp(output);

  _lastError = error;
  _lastOutput = output;
  return output;
}

void PidController::setTunings(float kp, float ki, float kd) {
  _kp = kp;
  _ki = ki;
  _kd = kd;
}

void PidController::setOutputLimits(float min, float max) {
  if (min > max) {
    const float tmp = min;
    min = max;
    max = tmp;
  }
  _outMin = min;
  _outMax = max;
  _limitsEnabled = true;
  _integral = clamp(_integral);
  _lastOutput = clamp(_lastOutput);
}

void PidController::reset() {
  _integral = 0.0f;
  _lastError = 0.0f;
  _lastOutput = 0.0f;
}

float PidController::clamp(float value) const {
  if (!_limitsEnabled) {
    return value;
  }
  if (value > _outMax) {
    return _outMax;
  }
  if (value < _outMin) {
    return _outMin;
  }
  return value;
}
