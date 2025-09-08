#pragma once

#include "blkhurst/engine/config/assets.hpp"
#include "blkhurst/engine/config/ui.hpp"
#include <blkhurst/engine/config/logger.hpp>
#include <blkhurst/engine/config/window.hpp>

namespace blkhurst {

struct EngineConfig {
  AssetsConfig assetsConfig{};
  LoggerConfig loggerConfig{};
  WindowConfig windowConfig{};
  UiConfig uiConfig{};
};

} // namespace blkhurst
