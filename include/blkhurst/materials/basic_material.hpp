#pragma once

#include <blkhurst/materials/material.hpp>
#include <blkhurst/materials/uv_transform.hpp>
#include <blkhurst/textures/cube_texture.hpp>
#include <blkhurst/textures/texture.hpp>

#include <glm/glm.hpp>
#include <memory>

namespace blkhurst {

enum class EnvMode : int { Reflection = 0, Refraction = 1 };

struct BasicMaterialDesc {
  glm::vec4 color{1.0F, 1.0F, 1.0F, 1.0F};
  std::shared_ptr<Texture> colorMap;
  std::shared_ptr<Texture> alphaMap;
  std::shared_ptr<Texture> normalMap;
  std::shared_ptr<CubeTexture> envMap;
  EnvMode envMode = EnvMode::Reflection;
  float reflectivity = 1.0F;
  float refractionRatio = 0.98F;
  bool flatShading = false;
  bool vertexColors = false;
};

class BasicMaterial : public Material {
public:
  BasicMaterial(const BasicMaterialDesc& desc = {});

  static std::shared_ptr<BasicMaterial> create(const BasicMaterialDesc& desc = {}) {
    return std::make_shared<BasicMaterial>(desc);
  }

  void setColor(const glm::vec3& rgb);
  void setColor(const glm::vec4& rgba);

  void setColorMap(std::shared_ptr<Texture> texture);
  void setAlphaMap(std::shared_ptr<Texture> texture);

  void setNormalMap(std::shared_ptr<Texture> texture);
  void setNormalScale(float scale);

  void setEnvMap(std::shared_ptr<CubeTexture> texture);
  void setEnvMode(EnvMode mode);

  void setFlatShading(bool enabled);
  void setVertexColors(bool enabled);

  void setReflectivity(float reflectivity);
  void setRefractionRatio(float refractionRatio);

  void setUvRepeat(float uRepeat, float vRepeat);
  void setUvOffset(const glm::vec2& offset);
  void setUvRotation(float radians);
  void setUvCenter(const glm::vec2& center);

protected:
  void applyResources() override;

private:
  glm::vec4 color_;

  std::shared_ptr<Texture> map_;
  std::shared_ptr<Texture> alphaMap_;

  std::shared_ptr<Texture> normalMap_;
  float normalScale_ = 1.0F;

  std::shared_ptr<CubeTexture> envMap_;
  EnvMode envMode_;

  bool flatShading_;
  bool vertexColors_;

  float reflectivity_;
  float refractionRatio_;

  UvTransform uvTransform_;
};

} // namespace blkhurst
