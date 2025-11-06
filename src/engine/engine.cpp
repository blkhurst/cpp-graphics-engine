#include "engine/engine_impl.hpp"
#include "logging/logger.hpp"
#include <blkhurst/shaders/shader_registry.hpp>
#include <blkhurst/util/assets.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

namespace blkhurst {

Engine::Engine(const EngineConfig& config) {
  // Configure Logger and Assets
  Logger logger_(config.logger.level);
  assets::setInstallRoot(config.assets.installRoot);
  assets::setSearchPaths(config.assets.searchPaths);

  // Register Builtin Shaders
  ShaderRegistry::registerBuiltinShaders();

  // Initialise Engine
  spdlog::stopwatch stopWatch;
  impl_ = std::make_unique<Impl>(config);
  spdlog::info("Engine initialised successfully in {:.2}s", stopWatch);
}

Engine::~Engine() {
  spdlog::info("Engine stopping...");
};

void Engine::run() {
  spdlog::info("Engine running...");
  impl_->run();
}

void Engine::setScene(const std::string& name) {
  impl_->setScene(name);
}

void Engine::registerSceneFactory(const std::string& name,
                                  std::function<std::unique_ptr<Scene>()> factory) {
  impl_->registerSceneFactory(name, std::move(factory));
}

} // namespace blkhurst
