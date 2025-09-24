#pragma once

#include <blkhurst/materials/material.hpp>
#include <blkhurst/textures/cube_texture.hpp>
#include <blkhurst/textures/texture.hpp>

#include <glm/glm.hpp>
#include <memory>

namespace blkhurst {

struct SkyBoxMaterialDesc {
  std::shared_ptr<CubeTexture> cubeMap;
  glm::mat3 rotation{1.0F};
  bool flipCubeMap =
      true; // https://github.com/mrdoob/three.js/blob/25e4e946fa2e69240a36c1263477aa0188231706/src/textures/CubeTexture.js#L14-L24
  float intensity = 1.0F;
};

class SkyBoxMaterial : public Material {
public:
  SkyBoxMaterial(const SkyBoxMaterialDesc& desc = {});
  ~SkyBoxMaterial() override = default;

  SkyBoxMaterial(const SkyBoxMaterial&) = delete;
  SkyBoxMaterial(SkyBoxMaterial&&) = delete;
  SkyBoxMaterial& operator=(const SkyBoxMaterial&) = delete;
  SkyBoxMaterial& operator=(SkyBoxMaterial&&) = delete;

  static std::shared_ptr<SkyBoxMaterial> create(const SkyBoxMaterialDesc& desc = {}) {
    return std::make_shared<SkyBoxMaterial>(desc);
  }

  void setCubeMap(std::shared_ptr<CubeTexture> cubemap);
  void setCubeMapRotation(const glm::mat3& rotation);
  void setFlipCubeMap(bool enabled);
  void setIntensity(float intensity);

  std::shared_ptr<Material> clone() const override;

protected:
  void applyResources() override;

private:
  std::shared_ptr<CubeTexture> cubeMap_;
  glm::mat3 cubeMapRotation_;
  bool flipCubeMap_;
  float intensity_;
};

} // namespace blkhurst
