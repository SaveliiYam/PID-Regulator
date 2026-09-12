#pragma once

/** Состояние автотюнера. */
enum class PidAutotuneState {
  Idle,
  Running,
  Finished,
  Failed
};

/**
 * Идентификаторы встроенных правил Ziegler–Nichols.
 * Кастомные формулы — через IPidTuningRule, без расширения этого enum.
 */
enum class PidAutotuneRule {
  ClassicPid,
  PessenIntegral,
  SomeOvershoot,
  NoOvershoot,
  PiOnly
};
