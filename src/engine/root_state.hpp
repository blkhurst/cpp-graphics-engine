#pragma once

#include <span>
#include <string>

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

  Renderer* renderer = nullptr;
  Camera* camera = nullptr;
  Input* input = nullptr;
  Scene* scene = nullptr;

  int currentSceneIndex = -1;
  std::span<const std::string> sceneNames;
};

} // namespace blkhurst
