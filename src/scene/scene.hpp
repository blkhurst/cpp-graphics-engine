#pragma once

#include "cameras/ortho_camera.hpp"
#include "objects/object3d.hpp"
#include "ui/ui_entry.hpp"
#include <blkhurst/engine/config/defaults.hpp>

#include <glm/glm.hpp>
#include <vector>

namespace blkhurst {

class Controller; // Forward

class Scene : public Object3D {
public:
  Scene();
  ~Scene() override = default;

  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  Scene(Scene&&) = delete;
  Scene& operator=(Scene&&) = delete;

  NodeKind kind() const override {
    return NodeKind::Object;
  }

  void onUpdate(const RootState& /*state*/) override {
  }

  [[nodiscard]] glm::vec4 background() const;
  [[nodiscard]] Camera* activeCamera() const;
  [[nodiscard]] Controller* activeController() const;
  [[nodiscard]] const std::vector<std::shared_ptr<UiEntry>>& uiEntries() const;

  void setBackground(const glm::vec4& backgroundVariant);
  void setActiveCamera(std::shared_ptr<Camera> camera);
  void setActiveController(std::shared_ptr<Controller> controller);
  void addUiEntry(std::shared_ptr<UiEntry> entry);

private:
  glm::vec4 background_ = defaults::window::clearColor;
  std::shared_ptr<Camera> activeCamera_ = std::make_shared<OrthoCamera>();
  std::shared_ptr<Controller> activeController_ = nullptr;
  std::vector<std::shared_ptr<UiEntry>> uiEntries_;

  // TODO: using Background = std::variant<glm::vec4, Texture, CubeTexture>;
  // TODO: add EnvironmentVariant environment
};

} // namespace blkhurst
