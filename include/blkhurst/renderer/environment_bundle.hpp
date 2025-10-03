#pragma once
#include <blkhurst/textures/cube_texture.hpp>
#include <blkhurst/textures/texture.hpp>

#include <glm/glm.hpp>
#include <memory>

namespace blkhurst {

struct EnvironmentBundle {
  std::shared_ptr<CubeTexture> environmentMap; // Original HDRI
  std::shared_ptr<Texture> brdfLUT;
  std::shared_ptr<CubeTexture> irradianceMap;
  std::shared_ptr<CubeTexture> prefilterMap;
  glm::mat3 rotation{1.0F};
  float intensity = 1.0F;

  [[nodiscard]] bool valid() const {
    return environmentMap && brdfLUT && irradianceMap && prefilterMap;
  }
};

} // namespace blkhurst
