#pragma once

#include <blkhurst/materials/material.hpp>
#include <blkhurst/materials/uv_transform.hpp>
#include <blkhurst/textures/cube_texture.hpp>
#include <blkhurst/textures/texture.hpp>

#include <glm/glm.hpp>
#include <memory>

namespace blkhurst {

struct PbrMaterialDesc {
  glm::vec3 color{1.0F, 1.0F, 1.0F};
  float opacity = 1.0F;
  float alphaTest = -1.0F;

  std::shared_ptr<Texture> albedoMap;
  std::shared_ptr<Texture> alphaMap;
  std::shared_ptr<Texture> normalMap;
  std::shared_ptr<Texture> metalnessMap;
  std::shared_ptr<Texture> roughnessMap;
  std::shared_ptr<Texture> aoMap;
  std::shared_ptr<Texture> emissiveMap;

  float normalScale = 1.0F;
  float metalness = 0.0F;
  float roughness = 1.0F;
  float aoIntensity = 1.0F;
  glm::vec3 emissiveColor{0.0F, 0.0F, 0.0F};
  float emissiveIntensity = 1.0F;

  std::shared_ptr<Texture> brdfLUT;
  std::shared_ptr<CubeTexture> irradianceMap;
  std::shared_ptr<CubeTexture> prefilterMap;

  glm::mat3 envRotation{1.0F};
  float envIntensity = 1.0F;

  bool flatShading = false;
  bool vertexColors = false;

  UvTransform uvTransform_;
};

class PbrMaterial : public Material {
public:
  PbrMaterial(const PbrMaterialDesc& desc = {});

  static std::shared_ptr<PbrMaterial> create(const PbrMaterialDesc& desc = {}) {
    return std::make_shared<PbrMaterial>(desc);
  }

  [[nodiscard]] const PbrMaterialDesc& desc() const {
    return desc_;
  }

  void setColor(const glm::vec3& rgb);
  void setOpacity(float alpha);
  void setAlphaTest(float threshold);

  void setAlbedoMap(std::shared_ptr<Texture> texture);
  void setAlphaMap(std::shared_ptr<Texture> texture);
  void setNormalMap(std::shared_ptr<Texture> texture);
  void setMetalnessMap(std::shared_ptr<Texture> texture);
  void setRoughnessMap(std::shared_ptr<Texture> texture);
  void setAoMap(std::shared_ptr<Texture> texture);
  void setEmissiveMap(std::shared_ptr<Texture> texture);

  void setIrradianceMap(std::shared_ptr<CubeTexture> texture);
  void setPrefilterMap(std::shared_ptr<CubeTexture> texture);
  void setBrdfLUT(std::shared_ptr<Texture> texture);

  void setEnvironmentRotation(const glm::mat3& rotation);
  void setEnvironmentIntensity(float intensity);

  void setNormalScale(float scale);
  void setMetalness(float metalness);
  void setRoughness(float roughness);
  void setAoIntensity(float intensity);
  void setEmissiveColor(const glm::vec3& rgb);
  void setEmissiveIntensity(float intensity);

  void setFlatShading(bool enabled);
  void setVertexColors(bool enabled);

  void setUvTransform(const UvTransform& uvTransform);
  void setUvRepeat(const glm::vec2& repeat);
  void setUvOffset(const glm::vec2& offset);
  void setUvRotation(float radians);
  void setUvCenter(const glm::vec2& center);

  void applyEnvironment(const EnvironmentBundle& env) override;

protected:
  void applyResources() override;

private:
  PbrMaterialDesc desc_;
};

} // namespace blkhurst
