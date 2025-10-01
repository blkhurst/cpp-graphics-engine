#pragma once

#include "blkhurst/materials/uniforms.hpp"
#include <blkhurst/materials/material.hpp>
#include <blkhurst/textures/cube_texture.hpp>

namespace blkhurst {

class IrradianceMaterial : public Material {
public:
  IrradianceMaterial(std::shared_ptr<CubeTexture> env, int faceSize)
      : Material(Program::createFromRegistry({
            .vert = "fullscreen_vert",
            .frag = "irradiance_frag",
        })),
        env_(std::move(env)),
        faceSize_(faceSize) {
  }

  static std::shared_ptr<IrradianceMaterial> create(std::shared_ptr<CubeTexture> env,
                                                    int faceSize) {
    return std::make_shared<IrradianceMaterial>(env, faceSize);
  }

  void setFace(int face) {
    face_ = face;
  }

  void setFaceSize(int size) {
    faceSize_ = size;
  }

protected:
  void applyResources() override {
    setUniform("uFace", face_);
    setUniform("uFaceSize", faceSize_);

    bindTextureUnit(env_, samplers::EnvMap, slots::EnvMap);
  }

private:
  std::shared_ptr<CubeTexture> env_;
  int face_ = 0;
  int faceSize_;
};

} // namespace blkhurst
