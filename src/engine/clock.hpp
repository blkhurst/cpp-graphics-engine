#pragma once

namespace blkhurst {

struct ClockInfo {
  float fps;
  float ms;
  float delta;
  float elapsed;
};

class Clock {
public:
  Clock();

  [[nodiscard]] ClockInfo tick();

  static float now();
  static void setNow(float seconds);

private:
  static constexpr float kMsPerSec = 1000.0;
  static constexpr float kSamplePeriod = 1.0 / 30.0;

  float startTime_ = 0.0;
  float currentTime_ = 0.0;
  float fpsPrevTime_ = 0.0;
  float dtPrevTime_ = 0.0;
  unsigned int frameCounter_ = 0;

  float fps_ = 0.0;
  float ms_ = 0.0;
  float delta_ = 0.0;
};

} // namespace blkhurst
