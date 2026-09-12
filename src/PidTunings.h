#pragma once

/** Набор коэффициентов ПИД (value object). */
struct PidTunings {
  float kp = 0.0f;
  float ki = 0.0f;
  float kd = 0.0f;

  PidTunings() = default;
  PidTunings(float kp_, float ki_, float kd_) : kp(kp_), ki(ki_), kd(kd_) {}
};
