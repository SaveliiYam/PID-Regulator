#pragma once

#include "PidTunings.h"

/**
 * Стратегия перевода Ku/Pu в Kp/Ki/Kd (Open/Closed для правил автотюна).
 */
class IPidTuningRule {
public:
  virtual ~IPidTuningRule() = default;
  virtual PidTunings compute(float ku, float pu) const = 0;
};
