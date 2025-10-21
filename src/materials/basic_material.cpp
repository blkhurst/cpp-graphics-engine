#include <blkhurst/materials/basic_material.hpp>
#include <blkhurst/materials/uniforms.hpp>

#include <spdlog/spdlog.h>

namespace blkhurst {
BasicMaterial::BasicMaterial(const BasicMaterialDesc& desc)
    : Material(Program::createFromRegistry({.vert = "basic_vert", .frag = "basic_frag"})),
      desc_(desc) {
  setColor(desc.color);
  setOpacity(desc.opacity);
  setAlphaTest(desc.alphaTest);

  setColorMap(desc.colorMap);
  setAlphaMap(desc.alphaMap);

  setNormalMap(desc.normalMap);
  setNormalScale(desc.normalScale);

  setEnvMap(desc.envMap);
  setEnvMode(desc.envMode);
  setReflectivity(desc.reflectivity);
  setRefractionRatio(desc.refractionRatio);

  setFlatShading(desc.flatShading);
  setVertexColors(desc.vertexColors);

  setUvTransform(desc.uvTransform_);
  spdlog::trace("BasicMaterial({}) created with Program({})", uuidString(),
                program()->uuidString());
}

const BasicMaterialDesc& BasicMaterial::desc() const {
  return desc_;
}

void BasicMaterial::setColor(const glm::vec3& rgb) {
  desc_.color = rgb;
}

void BasicMaterial::setOpacity(float alpha) {
  desc_.opacity = alpha;
}

void BasicMaterial::setAlphaTest(float threshold) {
  desc_.alphaTest = threshold;
}

void BasicMaterial::setColorMap(std::shared_ptr<Texture> texture) {
  desc_.colorMap = std::move(texture);
  setDefine(defines::UseColorMap, static_cast<bool>(desc_.colorMap));
}

void BasicMaterial::setAlphaMap(std::shared_ptr<Texture> texture) {
  desc_.alphaMap = std::move(texture);
  setDefine(defines::UseAlphaMap, static_cast<bool>(desc_.alphaMap));
}

void BasicMaterial::setNormalMap(std::shared_ptr<Texture> texture) {
  desc_.normalMap = std::move(texture);
  setDefine(defines::UseNormalMap, static_cast<bool>(desc_.normalMap));
}

void BasicMaterial::setNormalScale(float scale) {
  desc_.normalScale = scale;
}

void BasicMaterial::setEnvMap(std::shared_ptr<CubeTexture> texture) {
  desc_.envMap = std::move(texture);
  setDefine(defines::UseEnvMap, static_cast<bool>(desc_.envMap));
}

void BasicMaterial::setEnvMode(EnvMode mode) {
  desc_.envMode = mode;
  setDefine(defines::EnvModeReflection, desc_.envMode == EnvMode::Reflection);
}

void BasicMaterial::setFlatShading(bool enabled) {
  desc_.flatShading = enabled;
  setDefine(defines::UseFlatShading, desc_.flatShading);
}

void BasicMaterial::setVertexColors(bool enabled) {
  desc_.vertexColors = enabled;
  setDefine(defines::UseVertexColor, desc_.vertexColors);
}

void BasicMaterial::setReflectivity(float reflectivity) {
  desc_.reflectivity = reflectivity;
}

void BasicMaterial::setRefractionRatio(float refractionRatio) {
  desc_.refractionRatio = refractionRatio;
}

void BasicMaterial::setUvTransform(const UvTransform& uvTransform) {
  desc_.uvTransform_ = uvTransform;
  setDefine(defines::UseUvTransform, !desc_.uvTransform_.isDefault());
}

void BasicMaterial::setUvRepeat(const glm::vec2& repeat) {
  desc_.uvTransform_.setRepeat(repeat);
  setDefine(defines::UseUvTransform, !desc_.uvTransform_.isDefault());
}

void BasicMaterial::setUvOffset(const glm::vec2& offset) {
  desc_.uvTransform_.setOffset(offset);
  setDefine(defines::UseUvTransform, !desc_.uvTransform_.isDefault());
}

void BasicMaterial::setUvRotation(float radians) {
  desc_.uvTransform_.setRotation(radians);
  setDefine(defines::UseUvTransform, !desc_.uvTransform_.isDefault());
}

void BasicMaterial::setUvCenter(const glm::vec2& center) {
  desc_.uvTransform_.setCenter(center);
  setDefine(defines::UseUvTransform, !desc_.uvTransform_.isDefault());
}

void BasicMaterial::applyResources() {
  setUniform(uniforms::Color, desc_.color);
  setUniform(uniforms::Opacity, desc_.opacity);
  setUniform(uniforms::AlphaTest, desc_.alphaTest);
  setUniform(uniforms::NormalScale, desc_.normalScale);
  setUniform(uniforms::Reflectivity, desc_.reflectivity);
  setUniform(uniforms::RefractionRatio, desc_.refractionRatio);
  setUniform(uniforms::UvTransform, desc_.uvTransform_.matrix());

  bindTextureUnit(desc_.colorMap, samplers::ColorMap, slots::ColorMap);
  bindTextureUnit(desc_.alphaMap, samplers::AlphaMap, slots::AlphaMap);
  bindTextureUnit(desc_.normalMap, samplers::NormalMap, slots::NormalMap);
  bindTextureUnit(desc_.envMap, samplers::EnvMap, slots::EnvMap);
}

} // namespace blkhurst
