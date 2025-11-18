#include <blkhurst/postprocessing/pass.hpp>

namespace blkhurst {

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Pass::setSize(int width, int height) {
}

bool Pass::isEnabled() const {
  return enabled_;
}

void Pass::setEnabled(bool enabled) {
  enabled_ = enabled;
}

bool Pass::needsSwap() const {
  return needsSwap_;
}

void Pass::setNeedsSwap(bool needsSwap) {
  needsSwap_ = needsSwap;
}

} // namespace blkhurst
