#pragma once
#include <blkhurst/materials/material.hpp>
#include <blkhurst/textures/texture.hpp>

#include <memory>

namespace blkhurst {

struct EquirectMaterialDesc {
  std::shared_ptr<Texture> equirectTexture;
};

class EquirectMaterial : public Material {
public:
  EquirectMaterial(const EquirectMaterialDesc& desc = {});
  static std::shared_ptr<EquirectMaterial> create(const EquirectMaterialDesc& desc = {});
  void setFace(int face);

protected:
  void applyResources() override;

private:
  std::shared_ptr<Texture> equirectTexture_;
  int face_ = 0; // [0..5] (+X,-X,+Y,-Y,+Z,-Z)
};

} // namespace blkhurst
