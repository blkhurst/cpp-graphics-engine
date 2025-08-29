#include <blkhurst/engine.hpp>
#include <blkhurst/engine/config.hpp>

int main() {
  blkhurst::EngineConfig engineConfig;
  engineConfig.loggerConfig.level = blkhurst::LogLevel::debug;
  engineConfig.windowConfig.title = "Blkhurst Example";

  blkhurst::Engine engine{engineConfig};
  engine.run();
  return 0;
}
