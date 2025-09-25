#include <blkhurst/materials/basic_material.hpp>
#include <blkhurst/materials/uniforms.hpp>
#include <glm/fwd.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {
BasicMaterial::BasicMaterial(const BasicMaterialDesc& desc)
    : Material(Program::createFromRegistry({.vert = "basic_vert", .frag = "basic_frag"})),
      color_(desc.color),
      map_(desc.colorMap),
      alphaMap_(desc.alphaMap),
      envMap_(desc.envMap),
      envMode_(desc.envMode),
      reflectivity_(desc.reflectivity),
      refractionRatio_(desc.refractionRatio),
      flatShading_(desc.flatShading),
      vertexColors_(desc.vertexColors) {
  setColor(desc.color);
  setColorMap(desc.colorMap);
  setAlphaMap(desc.alphaMap);
  setEnvMap(desc.envMap);
  setEnvMode(desc.envMode);
  setReflectivity(desc.reflectivity);
  setRefractionRatio(desc.refractionRatio);
  setFlatShading(desc.flatShading);
  setVertexColors(desc.vertexColors);
  spdlog::trace("BasicMaterial created with Program({})", program()->id());
}

void BasicMaterial::setColor(const glm::vec3& rgb) {
  color_ = glm::vec4(rgb, 1.0F);
}

void BasicMaterial::setColor(const glm::vec4& rgba) {
  color_ = rgba;
}

void BasicMaterial::setColorMap(std::shared_ptr<Texture> texture) {
  map_ = std::move(texture);
  setDefine(defines::UseColorMap, static_cast<bool>(map_));
}

void BasicMaterial::setAlphaMap(std::shared_ptr<Texture> texture) {
  alphaMap_ = std::move(texture);
  setDefine(defines::UseAlphaMap, static_cast<bool>(alphaMap_));
}

void BasicMaterial::setEnvMap(std::shared_ptr<CubeTexture> texture) {
  envMap_ = std::move(texture);
  setDefine(defines::UseEnvMap, static_cast<bool>(envMap_));
}

void BasicMaterial::setEnvMode(EnvMode mode) {
  envMode_ = mode;
  setDefine(defines::EnvModeReflection, envMode_ == EnvMode::Reflection);
}

void BasicMaterial::setFlatShading(bool enabled) {
  flatShading_ = enabled;
  setDefine(defines::UseFlatShading, flatShading_);
}

void BasicMaterial::setVertexColors(bool enabled) {
  vertexColors_ = enabled;
  setDefine(defines::UseVertexColor, vertexColors_);
}

void BasicMaterial::setReflectivity(float reflectivity) {
  reflectivity_ = reflectivity;
}

void BasicMaterial::setRefractionRatio(float refractionRatio) {
  refractionRatio_ = refractionRatio;
}

void BasicMaterial::applyResources() {
  setUniform(uniforms::Color, color_);
  setUniform(uniforms::Reflectivity, reflectivity_);
  setUniform(uniforms::RefractionRatio, refractionRatio_);

  bindTextureUnit(map_, samplers::ColorMap, slots::ColorMap);
  bindTextureUnit(alphaMap_, samplers::AlphaMap, slots::AlphaMap);
  bindTextureUnit(envMap_, samplers::EnvMap, slots::EnvMap);
}

} // namespace blkhurst
