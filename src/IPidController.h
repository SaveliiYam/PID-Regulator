#pragma once

/**
 * Интерфейс ПИД-регулятора.
 * Позволяет подменять реализации в проектах без изменения вызывающего кода.
 */
class IPidController {
public:
  virtual ~IPidController() = default;

  /**
   * Вычисляет управляющее воздействие.
   * @param setpoint    заданное значение
   * @param measurement текущее измерение (обратная связь)
   * @param dt          шаг по времени, сек (должен быть > 0)
   * @return выход регулятора (обычно в тех же единицах, что и лимиты выхода)
   */
  virtual float compute(float setpoint, float measurement, float dt) = 0;

  virtual void setTunings(float kp, float ki, float kd) = 0;
  virtual void setOutputLimits(float min, float max) = 0;

  /** Сброс интегральной и дифференциальной составляющих */
  virtual void reset() = 0;

  virtual float getKp() const = 0;
  virtual float getKi() const = 0;
  virtual float getKd() const = 0;
  virtual float getLastOutput() const = 0;
  virtual float getLastError() const = 0;
};
