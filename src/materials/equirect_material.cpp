#include "materials/equirect_material.hpp"

#include <spdlog/spdlog.h>

namespace blkhurst {

EquirectMaterial::EquirectMaterial(const EquirectMaterialDesc& desc)
    : Material(Program::createFromRegistry({.vert = "equirect_vert", .frag = "equirect_frag"})),
      equirectTexture_(desc.equirectTexture) {
}

std::shared_ptr<EquirectMaterial> EquirectMaterial::create(const EquirectMaterialDesc& desc) {
  return std::make_shared<EquirectMaterial>(desc);
}

void EquirectMaterial::setFace(int face) {
  face_ = face;

  const int faceCount = 6;
  if (face_ < 0 || face_ >= faceCount) {
    face_ = 0;
    spdlog::warn("EquirectMaterial::setFace invalid face {}", face_);
  }
}

void EquirectMaterial::applyResources() {
  setUniform("uFace", face_);

  bindTextureUnit(equirectTexture_, "uEquirectT", 0);
}

} // namespace blkhurst
