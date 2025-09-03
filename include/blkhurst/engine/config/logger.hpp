#pragma once

#include <blkhurst/engine/config/defaults.hpp>
#include <blkhurst/engine/config/types.hpp>

namespace blkhurst {

struct LoggerConfig {
  LogLevel level{defaults::logger::logLevel};
};

} // namespace blkhurst
