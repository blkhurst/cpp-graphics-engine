#pragma once

#include <blkhurst/events/event_bus.hpp>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace blkhurst {

class Renderer;
class Camera;
class Scene;
class Input;

struct RootState {
  float delta = 0.0F;
  float elapsed = 0.0F;
  float fps = 0.0F;
  float ms = 0.0F;

  glm::vec2 windowFramebufferSize{0.0F};

  Renderer* renderer = nullptr;
  Camera* camera = nullptr;
  Input* input = nullptr;
  Scene* scene = nullptr;
  EventBus* events = nullptr;

  int currentSceneIndex = -1;
  std::vector<std::string> sceneNames;
};

} // namespace blkhurst
