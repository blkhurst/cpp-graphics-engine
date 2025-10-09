#include <blkhurst/materials/pbr_material.hpp>
#include <blkhurst/materials/uniforms.hpp>

#include <spdlog/spdlog.h>

namespace blkhurst {
PbrMaterial::PbrMaterial(const PbrMaterialDesc& desc)
    : Material(Program::createFromRegistry({.vert = "pbr_vert", .frag = "pbr_frag"})),
      desc_(desc) {
  setColor(desc.color);
  setOpacity(desc.opacity);
  setAlphaTest(desc.alphaTest);

  setAlbedoMap(desc.albedoMap);
  setAlphaMap(desc.alphaMap);
  setNormalMap(desc.normalMap);
  setMetalnessMap(desc.metalnessMap);
  setRoughnessMap(desc.roughnessMap);
  setAoMap(desc.aoMap);
  setEmissiveMap(desc.emissiveMap);

  setNormalScale(desc.normalScale);
  setMetalness(desc.metalness);
  setRoughness(desc.roughness);
  setAoIntensity(desc.aoIntensity);
  setEmissiveColor(desc.emissiveColor);
  setEmissiveIntensity(desc.emissiveIntensity);

  setBrdfLUT(desc.brdfLUT);
  setIrradianceMap(desc.irradianceMap);
  setPrefilterMap(desc.prefilterMap);

  setFlatShading(desc.flatShading);
  setVertexColors(desc.vertexColors);

  setUvTransform(desc_.uvTransform_);
  spdlog::trace("PbrMaterial created with Program({})", program()->id());
}

void PbrMaterial::setColor(const glm::vec3& rgb) {
  desc_.color = rgb;
}

void PbrMaterial::setOpacity(float alpha) {
  desc_.opacity = alpha;
}

void PbrMaterial::setAlphaTest(float threshold) {
  desc_.alphaTest = threshold;
}

void PbrMaterial::setAlbedoMap(std::shared_ptr<Texture> texture) {
  desc_.albedoMap = std::move(texture);
  setDefine(defines::UseColorMap, static_cast<bool>(desc_.albedoMap));
}

void PbrMaterial::setAlphaMap(std::shared_ptr<Texture> texture) {
  desc_.alphaMap = std::move(texture);
  setDefine(defines::UseAlphaMap, static_cast<bool>(desc_.alphaMap));
}

void PbrMaterial::setNormalMap(std::shared_ptr<Texture> texture) {
  desc_.normalMap = std::move(texture);
  setDefine(defines::UseNormalMap, static_cast<bool>(desc_.normalMap));
}

void PbrMaterial::setMetalnessMap(std::shared_ptr<Texture> texture) {
  desc_.metalnessMap = std::move(texture);
  setDefine(defines::UseMetalnessMap, static_cast<bool>(desc_.metalnessMap));
  if (desc_.metalnessMap) {
    desc_.metalness = 1.0F;
  }
}

void PbrMaterial::setRoughnessMap(std::shared_ptr<Texture> texture) {
  desc_.roughnessMap = std::move(texture);
  setDefine(defines::UseRoughnessMap, static_cast<bool>(desc_.roughnessMap));
  if (desc_.roughnessMap) {
    desc_.roughness = 1.0F;
  }
}

void PbrMaterial::setAoMap(std::shared_ptr<Texture> texture) {
  desc_.aoMap = std::move(texture);
  setDefine(defines::UseAoMap, static_cast<bool>(desc_.aoMap));
  if (desc_.aoMap) {
    desc_.aoIntensity = 1.0F;
  }
}

void PbrMaterial::setEmissiveMap(std::shared_ptr<Texture> texture) {
  desc_.emissiveMap = std::move(texture);
  setDefine(defines::UseEmissiveMap, static_cast<bool>(desc_.emissiveMap));
  if (desc_.emissiveMap) {
    desc_.emissiveColor = glm::vec3(1.0F);
  }
}

// IBL Controlled by uUseIBL, not defines
void PbrMaterial::setBrdfLUT(std::shared_ptr<Texture> texture) {
  desc_.brdfLUT = std::move(texture);
}

void PbrMaterial::setIrradianceMap(std::shared_ptr<CubeTexture> texture) {
  desc_.irradianceMap = std::move(texture);
}

void PbrMaterial::setPrefilterMap(std::shared_ptr<CubeTexture> texture) {
  desc_.prefilterMap = std::move(texture);
}

void PbrMaterial::setEnvironmentRotation(const glm::mat3& rotation) {
  desc_.envRotation = rotation;
}

void PbrMaterial::setEnvironmentIntensity(float intensity) {
  desc_.envIntensity = intensity;
}

void PbrMaterial::setNormalScale(float scale) {
  desc_.normalScale = scale;
}

void PbrMaterial::setMetalness(float metalness) {
  desc_.metalness = metalness;
}

void PbrMaterial::setRoughness(float roughness) {
  desc_.roughness = roughness;
}

void PbrMaterial::setAoIntensity(float intensity) {
  desc_.aoIntensity = intensity;
}

void PbrMaterial::setEmissiveColor(const glm::vec3& rgb) {
  desc_.emissiveColor = rgb;
}

void PbrMaterial::setEmissiveIntensity(float intensity) {
  desc_.emissiveIntensity = intensity;
}

void PbrMaterial::setFlatShading(bool enabled) {
  desc_.flatShading = enabled;
  setDefine(defines::UseFlatShading, desc_.flatShading);
}

void PbrMaterial::setVertexColors(bool enabled) {
  desc_.vertexColors = enabled;
  setDefine(defines::UseVertexColor, desc_.vertexColors);
}

void PbrMaterial::setUvTransform(const UvTransform& uvTransform) {
  desc_.uvTransform_ = uvTransform;
  setDefine(defines::UseUvTransform, !desc_.uvTransform_.isDefault());
}

void PbrMaterial::setUvRepeat(const glm::vec2& repeat) {
  desc_.uvTransform_.setRepeat(repeat);
  setDefine(defines::UseUvTransform, !desc_.uvTransform_.isDefault());
}

void PbrMaterial::setUvOffset(const glm::vec2& offset) {
  desc_.uvTransform_.setOffset(offset);
  setDefine(defines::UseUvTransform, !desc_.uvTransform_.isDefault());
}

void PbrMaterial::setUvRotation(float radians) {
  desc_.uvTransform_.setRotation(radians);
  setDefine(defines::UseUvTransform, !desc_.uvTransform_.isDefault());
}

void PbrMaterial::setUvCenter(const glm::vec2& center) {
  desc_.uvTransform_.setCenter(center);
  setDefine(defines::UseUvTransform, !desc_.uvTransform_.isDefault());
}

void PbrMaterial::applyEnvironment(const EnvironmentBundle& env) {
  // Select Material IBL or Fallback To Scene IBL
  auto brdf = desc_.brdfLUT ? desc_.brdfLUT : env.brdfLUT;
  auto irradiance = desc_.irradianceMap ? desc_.irradianceMap : env.irradianceMap;
  auto prefilter = desc_.prefilterMap ? desc_.prefilterMap : env.prefilterMap;
  auto envRotation = desc_.envRotation != glm::mat3(1.0F) ? desc_.envRotation : env.rotation;
  auto envIntensity = desc_.envIntensity != 1.0F ? desc_.envIntensity : env.intensity;

  const bool valid = (brdf && irradiance && prefilter);
  if (useIBL_ != valid) {
    useIBL_ = valid;
    setDefine(defines::UseIBL, valid);
  }

  setUniform(uniforms::EnvRotation, envRotation);
  setUniform(uniforms::EnvIntensity, envIntensity);

  bindTextureUnit(brdf, samplers::BrdfLUT, slots::BrdfLUT);
  bindTextureUnit(irradiance, samplers::IrradianceMap, slots::IrradianceMap);
  bindTextureUnit(prefilter, samplers::PrefilterMap, slots::PrefilterMap);
}

void PbrMaterial::applyResources() {
  setUniform(uniforms::Color, desc_.color);
  setUniform(uniforms::Opacity, desc_.opacity);
  setUniform(uniforms::AlphaTest, desc_.alphaTest);
  setUniform(uniforms::NormalScale, desc_.normalScale);
  setUniform(uniforms::Metalness, desc_.metalness);
  setUniform(uniforms::Roughness, desc_.roughness);
  setUniform(uniforms::AoIntensity, desc_.aoIntensity);
  setUniform(uniforms::EmissiveColor, desc_.emissiveColor);
  setUniform(uniforms::EmissiveIntensity, desc_.emissiveIntensity);

  setUniform(uniforms::UvTransform, desc_.uvTransform_.matrix());

  bindTextureUnit(desc_.albedoMap, samplers::ColorMap, slots::ColorMap);
  bindTextureUnit(desc_.alphaMap, samplers::AlphaMap, slots::AlphaMap);
  bindTextureUnit(desc_.normalMap, samplers::NormalMap, slots::NormalMap);
  bindTextureUnit(desc_.metalnessMap, samplers::MetalnessMap, slots::MetalnessMap);
  bindTextureUnit(desc_.roughnessMap, samplers::RoughnessMap, slots::RoughnessMap);
  bindTextureUnit(desc_.aoMap, samplers::AoMap, slots::AoMap);
  bindTextureUnit(desc_.emissiveMap, samplers::EmissiveMap, slots::EmissiveMap);
}

} // namespace blkhurst
