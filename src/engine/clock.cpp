#include "engine/clock.hpp"
#include <GLFW/glfw3.h>

namespace blkhurst {

Clock::Clock() {
  const float time = now();
  startTime_ = currentTime_ = fpsPrevTime_ = dtPrevTime_ = time;
}

ClockInfo Clock::tick() {
  currentTime_ = static_cast<float>(glfwGetTime());

  const float elapsedSinceFps = currentTime_ - fpsPrevTime_;
  ++frameCounter_;

  if (elapsedSinceFps >= kSamplePeriod) {
    const auto frames = static_cast<float>(frameCounter_);
    fps_ = frames / elapsedSinceFps;
    ms_ = (elapsedSinceFps / frames) * kMsPerSec;
    fpsPrevTime_ = currentTime_;
    frameCounter_ = 0;
  }

  // Per-frame delta
  delta_ = currentTime_ - dtPrevTime_;
  dtPrevTime_ = currentTime_;

  return ClockInfo{.fps = fps_, .ms = ms_, .delta = delta_, .elapsed = currentTime_ - startTime_};
}

float Clock::now() {
  return static_cast<float>(glfwGetTime());
}

void Clock::setNow(float seconds) {
  glfwSetTime(seconds);
}

} // namespace blkhurst
