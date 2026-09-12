#pragma once

#include "IPidController.h"

/**
 * Классический ПИД-регулятор с ограничением выхода и anti-windup.
 * Дифференциальная составляющая считается по ошибке.
 */
class PidController : public IPidController {
public:
  PidController(float kp = 1.0f, float ki = 0.0f, float kd = 0.0f);

  float compute(float setpoint, float measurement, float dt) override;

  void setTunings(float kp, float ki, float kd) override;
  void setOutputLimits(float min, float max) override;
  void reset() override;

  float getKp() const override { return _kp; }
  float getKi() const override { return _ki; }
  float getKd() const override { return _kd; }
  float getLastOutput() const override { return _lastOutput; }
  float getLastError() const override { return _lastError; }

  float getIntegral() const { return _integral; }

private:
  float clamp(float value) const;

  float _kp;
  float _ki;
  float _kd;

  float _integral;
  float _lastError;
  float _lastOutput;

  float _outMin;
  float _outMax;
  bool _limitsEnabled;
};
