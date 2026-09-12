#pragma once

/**
 * Ограничение выхода — общая ответственность для регулятора и автотюнера.
 */
class OutputClamp {
public:
  void setLimits(float min, float max) {
    if (min > max) {
      const float tmp = min;
      min = max;
      max = tmp;
    }
    _min = min;
    _max = max;
    _enabled = true;
  }

  bool isEnabled() const { return _enabled; }
  float min() const { return _min; }
  float max() const { return _max; }

  float apply(float value) const {
    if (!_enabled) {
      return value;
    }
    if (value > _max) {
      return _max;
    }
    if (value < _min) {
      return _min;
    }
    return value;
  }

private:
  float _min = 0.0f;
  float _max = 0.0f;
  bool _enabled = false;
};
