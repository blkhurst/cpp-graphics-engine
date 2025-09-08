#pragma once

#include <blkhurst/engine/config.hpp>
#include <blkhurst/scene/scene.hpp>
#include <memory>

namespace blkhurst {

class Engine {
public:
  Engine(const EngineConfig& config = {});
  ~Engine();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
  Engine(Engine&&) = delete;
  Engine& operator=(Engine&&) = delete;

  void run();

  template <class TScene, class... Args>
  void registerScene(const std::string& name, Args&&... args) {
    static_assert(std::is_base_of_v<Scene, TScene>, "TScene must derive from Scene");
    auto factory = [=]() -> std::unique_ptr<Scene> { return std::make_unique<TScene>(args...); };
    registerSceneFactory(name, std::move(factory));
  }

private:
  class Impl;
  std::unique_ptr<Impl> impl_;

  void registerSceneFactory(const std::string& name,
                            std::function<std::unique_ptr<Scene>()> factory);
};

} // namespace blkhurst
