#pragma once

#include <blkhurst/engine/config/logger.hpp>
#include <blkhurst/engine/config/window.hpp>

namespace blkhurst {

struct EngineConfig {
  LoggerConfig loggerConfig{};
  WindowConfig windowConfig{};
};

} // namespace blkhurst
