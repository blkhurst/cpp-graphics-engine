#include <blkhurst/materials/skybox_material.hpp>
#include <blkhurst/materials/slots.hpp>
#include <glm/glm.hpp>

namespace blkhurst {

SkyBoxMaterial::SkyBoxMaterial(const SkyBoxMaterialDesc& desc)
    : Material(Program::createFromRegistry({.vert = "skybox_vert", .frag = "skybox_frag"})),
      cubeMap_(desc.cubeMap),
      cubeMapRotation_(desc.rotation),
      flipCubeMap_(desc.flipCubeMap),
      intensity_(desc.intensity) {
  // Set pipeline state
  setCullFace(CullFace::Front);
  setDepthTest(true);
  setDepthFunc(DepthFunc::Lequal);
  setDepthWrite(false);
}

void SkyBoxMaterial::setCubeMap(std::shared_ptr<CubeTexture> cubemap) {
  cubeMap_ = std::move(cubemap);
}

void SkyBoxMaterial::setCubeMapRotation(const glm::mat3& rotation) {
  cubeMapRotation_ = rotation;
}

void SkyBoxMaterial::setFlipCubeMap(bool enabled) {
  flipCubeMap_ = enabled;
}

void SkyBoxMaterial::setIntensity(float intensity) {
  intensity_ = intensity;
}
std::shared_ptr<Material> SkyBoxMaterial::clone() const {
  return std::make_shared<SkyBoxMaterial>(SkyBoxMaterialDesc{.cubeMap = cubeMap_,
                                                             .rotation = cubeMapRotation_,
                                                             .flipCubeMap = flipCubeMap_,
                                                             .intensity = intensity_});
}

void SkyBoxMaterial::applyResources() {
  setUniform("uCubeMapRotation", cubeMapRotation_);
  setUniform("uFlipCubeMap", flipCubeMap_ ? -1.0F : +1.0F);
  setUniform("uIntensity", intensity_);

  bindTextureUnit(cubeMap_, "uCubeMap", 1);
}

} // namespace blkhurst
