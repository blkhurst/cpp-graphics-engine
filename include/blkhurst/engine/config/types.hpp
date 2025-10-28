#pragma once

namespace blkhurst {

// Logger
enum class LogLevel { trace, debug, info, warn, err, critical, off };

// Window
struct GLVersion {
  int major;
  int minor;
};

// Scenes
enum class SceneLoadPolicy {
  Preload,
  OnDemand,
  OnDemandUnloadInactive,
};

} // namespace blkhurst
