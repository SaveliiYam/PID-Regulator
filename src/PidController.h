#pragma once

#include "IPidController.h"
#include "OutputClamp.h"
#include "PidTunings.h"

/**
 * Классический ПИД-регулятор с ограничением выхода и anti-windup.
 * Дифференциальная составляющая считается по ошибке.
 */
class PidController : public IPidController {
public:
  PidController(float kp = 1.0f, float ki = 0.0f, float kd = 0.0f);
  explicit PidController(const PidTunings& tunings);

  float compute(float setpoint, float measurement, float dt) override;

  void setTunings(float kp, float ki, float kd) override;
  void setTunings(const PidTunings& tunings) override;
  void setOutputLimits(float min, float max) override;
  void reset() override;

  PidTunings getTunings() const override { return _tunings; }
  float getKp() const override { return _tunings.kp; }
  float getKi() const override { return _tunings.ki; }
  float getKd() const override { return _tunings.kd; }
  float getLastOutput() const override { return _lastOutput; }
  float getLastError() const override { return _lastError; }

  float getIntegral() const { return _integral; }

private:
  PidTunings _tunings;
  float _integral;
  float _lastError;
  float _lastOutput;
  OutputClamp _clamp;
};
