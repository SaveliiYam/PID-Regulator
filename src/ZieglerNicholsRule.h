#pragma once

#include "IPidTuningRule.h"
#include "PidAutotuneTypes.h"
#include "PidTunings.h"

/** Встроенные правила Ziegler–Nichols и варианты. */
class ZieglerNicholsRule : public IPidTuningRule {
public:
  explicit ZieglerNicholsRule(PidAutotuneRule rule);

  PidTunings compute(float ku, float pu) const override;

  static const IPidTuningRule& forRule(PidAutotuneRule rule);

private:
  PidAutotuneRule _rule;
};
