#pragma once

#include <blkhurst/cameras/ortho_camera.hpp>
#include <blkhurst/controllers/controller.hpp>
#include <blkhurst/engine/config/defaults.hpp>
#include <blkhurst/objects/object3d.hpp>
#include <blkhurst/textures/cube_texture.hpp>
#include <blkhurst/textures/texture.hpp>
#include <blkhurst/ui/ui_entry.hpp>

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace blkhurst {

enum class BackgroundType { Color, /*Texture,*/ Cube, Equirect };

struct SceneBackground {
  BackgroundType type = BackgroundType::Color;
  glm::vec4 color{defaults::window::clearColor};

  std::shared_ptr<Texture> texture;
  std::shared_ptr<CubeTexture> cubemap;

  float intensity = 1.0F;
};

// struct SceneEnvironment {
//   std::shared_ptr<CubeTexture> cubemap; // PMREM
//   glm::mat3 rotation{1.0F};
//   float intensity = 1.0F;
// };

class Scene : public Object3D {
public:
  Scene();
  ~Scene() override;

  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  Scene(Scene&&) = delete;
  Scene& operator=(Scene&&) = delete;

  NodeKind kind() const override {
    return NodeKind::Object;
  }

  void onUpdate(const RootState& /*state*/) override {
  }

  [[nodiscard]] const SceneBackground& background() const;
  void setBackground(const glm::vec4& color);
  void setBackground(std::shared_ptr<CubeTexture> cubemap);
  void setBackground(std::shared_ptr<Texture> equirect);
  void setBackgroundIntensity(float intensity);

  // Sets environment for PBR children only
  // [[nodiscard]] const SceneEnvironment& environment() const;
  // void setEnvironment(std::shared_ptr<Texture> equirect, bool setBackground = true);
  // void setEnvironmentIntensity(float intensity);
  // void setEnvironmentRotation(const glm::mat3& rotation);

  [[nodiscard]] Camera* activeCamera() const;
  [[nodiscard]] Controller* activeController() const;
  [[nodiscard]] const std::vector<std::shared_ptr<UiEntry>>& uiEntries() const;

  void setActiveCamera(std::shared_ptr<Camera> camera);
  void setActiveController(std::shared_ptr<Controller> controller);
  void addUiEntry(std::shared_ptr<UiEntry> entry);

private:
  SceneBackground background_{};
  // SceneEnvironment environment_{};

  std::shared_ptr<Camera> activeCamera_ = std::make_shared<OrthoCamera>();
  std::shared_ptr<Controller> activeController_ = nullptr;
  std::vector<std::shared_ptr<UiEntry>> uiEntries_;
};

} // namespace blkhurst
