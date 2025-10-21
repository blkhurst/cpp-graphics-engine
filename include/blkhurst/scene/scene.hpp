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

struct SceneEnvironment {
  // Outputs
  std::shared_ptr<Texture> brdfLUT;
  std::shared_ptr<CubeTexture> irradianceMap;
  std::shared_ptr<CubeTexture> prefilterMap;
  // Inputs
  std::shared_ptr<Texture> equirect;
  glm::mat3 rotation{1.0F};
  float intensity = 1.0F;

  bool setBackground = true;
  bool needsUpdate = false;
};

class Scene : public Object3D {
public:
  Scene();
  ~Scene() override;

  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  Scene(Scene&&) = delete;
  Scene& operator=(Scene&&) = delete;

  void onUpdate(const RootState& /*state*/) override {
  }

  [[nodiscard]] const SceneBackground& background() const;
  void setBackground(const glm::vec4& color);
  void setBackground(std::shared_ptr<CubeTexture> cubemap);
  void setBackground(std::shared_ptr<Texture> equirect);
  void setBackgroundIntensity(float intensity);

  // Currently supports PBR children only
  [[nodiscard]] SceneEnvironment& environment();
  void setEnvironment(std::shared_ptr<Texture> equirect, bool setBackground = true);
  void setEnvironmentIntensity(float intensity);
  void setEnvironmentRotation(const glm::mat3& rotation);

  [[nodiscard]] Camera* activeCamera() const;
  [[nodiscard]] Controller* activeController() const;
  [[nodiscard]] const std::vector<std::shared_ptr<UiEntry>>& uiEntries() const;

  void setActiveCamera(std::shared_ptr<Camera> camera);
  void setActiveController(std::shared_ptr<Controller> controller);
  void addUiEntry(std::shared_ptr<UiEntry> entry);

private:
  SceneBackground background_{};
  SceneEnvironment environment_{};

  std::shared_ptr<Camera> activeCamera_ = std::make_shared<OrthoCamera>();
  std::shared_ptr<Controller> activeController_ = nullptr;
  std::vector<std::shared_ptr<UiEntry>> uiEntries_;
};

} // namespace blkhurst
