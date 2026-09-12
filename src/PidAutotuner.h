#pragma once

#include "IPidAutotuner.h"
#include "IPidController.h"
#include "IPidTuningRule.h"
#include "OutputClamp.h"
#include "PidTunings.h"

/**
 * Автонастройка ПИД методом реле (Åström–Hägglund).
 * Отвечает за релейный эксперимент и измерение Ku/Pu;
 * перевод в Kp/Ki/Kd делегирует IPidTuningRule (Strategy).
 */
class PidAutotuner : public IPidAutotuner {
public:
  explicit PidAutotuner(IPidController& pid);

  void setTarget(float target) override;
  void setOutputStep(float step) override;
  void setNoiseBand(float band) override;
  void setControlRule(Rule rule) override;
  void setTuningRule(const IPidTuningRule& rule) override;
  void setIgnoreCycles(unsigned count) override;
  void setSettleCycles(unsigned count) override;
  void setTimeout(float seconds) override;
  void setOutputLimits(float min, float max) override;

  void start(float measurement, float outputCenter) override;
  void cancel() override;
  float update(float measurement, float dt) override;

  State getState() const override { return _state; }
  bool isRunning() const override { return _state == State::Running; }
  bool isFinished() const override { return _state == State::Finished; }
  bool isFailed() const override { return _state == State::Failed; }

  PidTunings getTunings() const override { return _tunings; }
  float getKp() const override { return _tunings.kp; }
  float getKi() const override { return _tunings.ki; }
  float getKd() const override { return _tunings.kd; }
  float getKu() const override { return _ku; }
  float getPu() const override { return _pu; }
  float getLastOutput() const override { return _lastOutput; }

  void applyTunings() override;

private:
  void onRelayEdge(float measurement);
  void finishSuccess();
  void finishFail();

  IPidController& _pid;
  const IPidTuningRule* _tuningRule;

  State _state;
  float _target;
  float _outputStep;
  float _noiseBand;
  float _outputCenter;
  OutputClamp _clamp;

  unsigned _ignoreCycles;
  unsigned _settleCycles;
  float _timeoutSec;

  bool _relayHigh;
  float _peakMax;
  float _peakMin;
  float _elapsed;
  float _lastEdgeTime;
  float _lastOutput;

  unsigned _edgesSeen;
  unsigned _periodsCollected;
  float _sumPeriod;
  float _sumAmplitude;

  PidTunings _tunings;
  float _ku;
  float _pu;
};
