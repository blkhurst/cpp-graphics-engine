#pragma once

#include <blkhurst/engine/config.hpp>
#include <memory>

namespace blkhurst {

class Engine {
public:
  Engine();
  Engine(const EngineConfig& config);
  ~Engine();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
  Engine(Engine&&) = delete;
  Engine& operator=(Engine&&) = delete;

  void run();

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace blkhurst
