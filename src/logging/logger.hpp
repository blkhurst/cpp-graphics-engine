#pragma once

#include <blkhurst/engine/config/logger.hpp>
#include <cstdlib>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

namespace blkhurst {

struct Logger {
  Logger(LogLevel logLevel) {
    // Set Log Level
    spdlog::cfg::load_env_levels();
    if (std::getenv("SPDLOG_LEVEL") == nullptr) {
      spdlog::set_level(static_cast<spdlog::level::level_enum>(logLevel));
    }

    // Set Pattern
    spdlog::set_pattern("[%T.%e] [%^%l%$] %v");
  }
};

} // namespace blkhurst
