#include "ZieglerNicholsRule.h"

ZieglerNicholsRule::ZieglerNicholsRule(PidAutotuneRule rule) : _rule(rule) {}

PidTunings ZieglerNicholsRule::compute(float ku, float pu) const {
  float kpFactor = 0.6f;
  float tiFactor = 0.5f;
  float tdFactor = 0.125f;

  switch (_rule) {
    case PidAutotuneRule::ClassicPid:
      kpFactor = 0.6f;
      tiFactor = 0.5f;
      tdFactor = 0.125f;
      break;
    case PidAutotuneRule::PessenIntegral:
      kpFactor = 0.7f;
      tiFactor = 0.4f;
      tdFactor = 0.15f;
      break;
    case PidAutotuneRule::SomeOvershoot:
      kpFactor = 0.33f;
      tiFactor = 0.5f;
      tdFactor = 0.33f;
      break;
    case PidAutotuneRule::NoOvershoot:
      kpFactor = 0.2f;
      tiFactor = 0.5f;
      tdFactor = 0.33f;
      break;
    case PidAutotuneRule::PiOnly:
      kpFactor = 0.45f;
      tiFactor = 0.83f;
      tdFactor = 0.0f;
      break;
  }

  PidTunings t;
  t.kp = kpFactor * ku;
  const float ti = tiFactor * pu;
  const float td = tdFactor * pu;
  t.ki = (ti > 1e-6f) ? (t.kp / ti) : 0.0f;
  t.kd = t.kp * td;
  return t;
}

const IPidTuningRule& ZieglerNicholsRule::forRule(PidAutotuneRule rule) {
  static const ZieglerNicholsRule classic(PidAutotuneRule::ClassicPid);
  static const ZieglerNicholsRule pessen(PidAutotuneRule::PessenIntegral);
  static const ZieglerNicholsRule someOvershoot(PidAutotuneRule::SomeOvershoot);
  static const ZieglerNicholsRule noOvershoot(PidAutotuneRule::NoOvershoot);
  static const ZieglerNicholsRule piOnly(PidAutotuneRule::PiOnly);

  switch (rule) {
    case PidAutotuneRule::PessenIntegral:
      return pessen;
    case PidAutotuneRule::SomeOvershoot:
      return someOvershoot;
    case PidAutotuneRule::NoOvershoot:
      return noOvershoot;
    case PidAutotuneRule::PiOnly:
      return piOnly;
    case PidAutotuneRule::ClassicPid:
    default:
      return classic;
  }
}
