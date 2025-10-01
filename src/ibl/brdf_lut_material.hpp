#pragma once

#include <blkhurst/materials/material.hpp>
#include <blkhurst/textures/cube_texture.hpp>

namespace blkhurst {

class BrdfLUTMaterial : public Material {
public:
  BrdfLUTMaterial()
      : Material(Program::createFromRegistry({
            .vert = "fullscreen_vert",
            .frag = "brdf_lut_frag",
        })) {
  }

  static std::shared_ptr<BrdfLUTMaterial> create() {
    return std::make_shared<BrdfLUTMaterial>();
  }
};

} // namespace blkhurst
