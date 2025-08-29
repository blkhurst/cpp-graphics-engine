#include "engine/clock.hpp"
#include "logging/logger.hpp"
#include <blkhurst/engine.hpp>
#include <blkhurst/engine/config.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

namespace blkhurst {

// PImpl
class Engine::Impl {
public:
  explicit Impl(const EngineConfig& cfg)
      : config_(cfg),
        logger_(cfg.loggerConfig.level) {
  }

  void run() {
    while (false) {
      const auto tick = clock_.tick();
    }
  }

private:
  EngineConfig config_;
  Logger logger_;
  Clock clock_;
};

// Public
Engine::Engine() {
  spdlog::stopwatch stopWatch;

  impl_ = std::make_unique<Impl>(EngineConfig{});
  spdlog::info("Engine initialised successfully {:.2}s (default config)", stopWatch);
}

Engine::Engine(const EngineConfig& config) {
  spdlog::stopwatch stopWatch;

  impl_ = std::make_unique<Impl>(config);
  spdlog::info("Engine initialised successfully {:.2}s (custom config)", stopWatch);
}

Engine::~Engine() {
  spdlog::info("Engine stopping...");
};

void Engine::run() {
  spdlog::info("Engine running...");
  impl_->run();
}

} // namespace blkhurst
