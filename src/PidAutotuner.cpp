#include "PidAutotuner.h"

#include "ZieglerNicholsRule.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PidAutotuner::PidAutotuner(IPidController& pid)
    : _pid(pid),
      _tuningRule(&ZieglerNicholsRule::forRule(Rule::ClassicPid)),
      _state(State::Idle),
      _target(0.0f),
      _outputStep(50.0f),
      _noiseBand(1.0f),
      _outputCenter(0.0f),
      _ignoreCycles(2),
      _settleCycles(6),
      _timeoutSec(60.0f),
      _relayHigh(true),
      _peakMax(0.0f),
      _peakMin(0.0f),
      _elapsed(0.0f),
      _lastEdgeTime(0.0f),
      _lastOutput(0.0f),
      _edgesSeen(0),
      _periodsCollected(0),
      _sumPeriod(0.0f),
      _sumAmplitude(0.0f),
      _tunings{},
      _ku(0.0f),
      _pu(0.0f) {}

void PidAutotuner::setTarget(float target) { _target = target; }

void PidAutotuner::setOutputStep(float step) {
  _outputStep = fabsf(step);
}

void PidAutotuner::setNoiseBand(float band) {
  _noiseBand = fabsf(band);
}

void PidAutotuner::setControlRule(Rule rule) {
  _tuningRule = &ZieglerNicholsRule::forRule(rule);
}

void PidAutotuner::setTuningRule(const IPidTuningRule& rule) {
  _tuningRule = &rule;
}

void PidAutotuner::setIgnoreCycles(unsigned count) { _ignoreCycles = count; }

void PidAutotuner::setSettleCycles(unsigned count) {
  _settleCycles = count < 4 ? 4 : count;
}

void PidAutotuner::setTimeout(float seconds) {
  _timeoutSec = seconds < 0.0f ? 0.0f : seconds;
}

void PidAutotuner::setOutputLimits(float min, float max) {
  _clamp.setLimits(min, max);
}

void PidAutotuner::start(float measurement, float outputCenter) {
  _outputCenter = outputCenter;
  _state = State::Running;
  _elapsed = 0.0f;
  _lastEdgeTime = 0.0f;
  _edgesSeen = 0;
  _periodsCollected = 0;
  _sumPeriod = 0.0f;
  _sumAmplitude = 0.0f;
  _tunings = {};
  _ku = _pu = 0.0f;

  _peakMax = measurement;
  _peakMin = measurement;

  const float error = _target - measurement;
  _relayHigh = (error >= 0.0f);

  _lastOutput =
      _clamp.apply(_outputCenter + (_relayHigh ? _outputStep : -_outputStep));
}

void PidAutotuner::cancel() {
  if (_state == State::Running) {
    _state = State::Idle;
  }
}

float PidAutotuner::update(float measurement, float dt) {
  if (_state != State::Running) {
    return _lastOutput;
  }
  if (dt <= 0.0f || !isfinite(dt)) {
    return _lastOutput;
  }

  _elapsed += dt;

  if (_timeoutSec > 0.0f && _elapsed >= _timeoutSec) {
    finishFail();
    return _lastOutput;
  }

  if (measurement > _peakMax) {
    _peakMax = measurement;
  }
  if (measurement < _peakMin) {
    _peakMin = measurement;
  }

  const float error = _target - measurement;

  if (_relayHigh) {
    if (error < -_noiseBand) {
      onRelayEdge(measurement);
      _relayHigh = false;
    }
  } else {
    if (error > _noiseBand) {
      onRelayEdge(measurement);
      _relayHigh = true;
    }
  }

  if (_state != State::Running) {
    return _lastOutput;
  }

  _lastOutput =
      _clamp.apply(_outputCenter + (_relayHigh ? _outputStep : -_outputStep));
  return _lastOutput;
}

void PidAutotuner::onRelayEdge(float measurement) {
  const float amplitude = (_peakMax - _peakMin) * 0.5f;
  const float halfPeriod = _elapsed - _lastEdgeTime;

  _edgesSeen++;

  if (_edgesSeen > 1 && halfPeriod > 0.0f) {
    const unsigned halfCycleIndex = _edgesSeen - 1;
    if (halfCycleIndex > _ignoreCycles) {
      _sumPeriod += halfPeriod;
      _sumAmplitude += amplitude;
      _periodsCollected++;

      if (_periodsCollected >= _settleCycles) {
        finishSuccess();
        return;
      }
    }
  }

  _lastEdgeTime = _elapsed;
  _peakMax = measurement;
  _peakMin = measurement;
}

void PidAutotuner::finishSuccess() {
  const float avgHalfPeriod =
      _sumPeriod / static_cast<float>(_periodsCollected);
  const float avgAmplitude =
      _sumAmplitude / static_cast<float>(_periodsCollected);

  if (avgHalfPeriod <= 0.0f || avgAmplitude < 1e-6f || _outputStep < 1e-6f ||
      _tuningRule == nullptr) {
    finishFail();
    return;
  }

  _pu = 2.0f * avgHalfPeriod;
  _ku = (4.0f * _outputStep) / (static_cast<float>(M_PI) * avgAmplitude);
  _tunings = _tuningRule->compute(_ku, _pu);
  _state = State::Finished;
}

void PidAutotuner::finishFail() {
  _state = State::Failed;
  _tunings = {};
  _ku = _pu = 0.0f;
}

void PidAutotuner::applyTunings() {
  if (_state != State::Finished) {
    return;
  }
  _pid.setTunings(_tunings);
  _pid.reset();
}
