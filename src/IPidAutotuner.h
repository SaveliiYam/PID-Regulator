#pragma once

#include "IPidTuningRule.h"
#include "PidAutotuneTypes.h"
#include "PidTunings.h"

/**
 * Интерфейс автонастройки ПИД-регулятора.
 * Зависит от абстракций (IPidTuningRule, PidTunings), а не от конкретной формулы.
 */
class IPidAutotuner {
public:
  using State = PidAutotuneState;
  using Rule = PidAutotuneRule;

  virtual ~IPidAutotuner() = default;

  virtual void setTarget(float target) = 0;
  /** Амплитуда реле относительно центра выхода (d в формуле Ku) */
  virtual void setOutputStep(float step) = 0;
  /** Полузона гистерезиса вокруг setpoint (шум) */
  virtual void setNoiseBand(float band) = 0;

  /** Встроенное правило (ClassicPid, PiOnly, …) */
  virtual void setControlRule(Rule rule) = 0;
  /**
   * Произвольная стратегия Ku/Pu → Kp/Ki/Kd.
   * Указатель должен жить дольше тюнера (обычно static/global).
   */
  virtual void setTuningRule(const IPidTuningRule& rule) = 0;

  virtual void setIgnoreCycles(unsigned count) = 0;
  virtual void setSettleCycles(unsigned count) = 0;
  virtual void setTimeout(float seconds) = 0;
  virtual void setOutputLimits(float min, float max) = 0;

  virtual void start(float measurement, float outputCenter) = 0;
  virtual void cancel() = 0;
  virtual float update(float measurement, float dt) = 0;

  virtual State getState() const = 0;
  virtual bool isRunning() const = 0;
  virtual bool isFinished() const = 0;
  virtual bool isFailed() const = 0;

  virtual PidTunings getTunings() const = 0;
  virtual float getKp() const = 0;
  virtual float getKi() const = 0;
  virtual float getKd() const = 0;
  virtual float getKu() const = 0;
  virtual float getPu() const = 0;
  virtual float getLastOutput() const = 0;

  /** Записать найденные коэффициенты в привязанный ПИД */
  virtual void applyTunings() = 0;
};
