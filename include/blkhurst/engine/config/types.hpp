#pragma once

namespace blkhurst {

// Logger
enum class LogLevel { trace, debug, info, warn, err, critical, off };

// Window
struct GLVersion {
  int major;
  int minor;
};

struct RGBA {
  float r = 0, g = 0, b = 0, a = 1;
};

} // namespace blkhurst
