#pragma once

#include "blkhurst/materials/uniforms.hpp"
#include <blkhurst/materials/material.hpp>
#include <blkhurst/textures/cube_texture.hpp>

namespace blkhurst {

class IrradianceMaterial : public Material {
public:
  IrradianceMaterial(std::shared_ptr<CubeTexture> env)
      : Material(Program::createFromRegistry({
            .vert = "fullscreen_vert",
            .frag = "irradiance_frag",
        })),
        env_(std::move(env)) {
  }

  static std::shared_ptr<IrradianceMaterial> create(std::shared_ptr<CubeTexture> env) {
    return std::make_shared<IrradianceMaterial>(env);
  }

  void setFace(int face) {
    face_ = face;
  }

protected:
  void applyResources() override {
    setUniform("uFace", face_);

    bindTextureUnit(env_, samplers::EnvMap, slots::EnvMap);
  }

private:
  std::shared_ptr<CubeTexture> env_;
  int face_ = 0;
};

} // namespace blkhurst
