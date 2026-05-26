#pragma once
#include "blkhurst/materials/uniforms.hpp"
#include "blkhurst/postprocessing/shaders/copy.glsl.hpp"
#include "blkhurst/shaders/builtin/fullscreen.glsl.hpp"
#include <blkhurst/materials/material.hpp>
#include <blkhurst/textures/texture.hpp>

#include <memory>

namespace blkhurst {

class CopyMaterial : public Material {
public:
  CopyMaterial()
      : Material(Program::create({.vert = shaders::fullscreen_vert, .frag = shaders::copy_frag})) {
    setDepthTest(false);
    setDepthWrite(false);
  }

  static std::shared_ptr<CopyMaterial> create() {
    return std::make_shared<CopyMaterial>();
  };

  void setInputBuffer(const std::shared_ptr<Texture>& texture) {
    inputBuffer_ = texture;
  }

protected:
  void applyResources() override {
    bindTextureUnit(inputBuffer_, samplers::InputBuffer, slots::Postprocessing0);
  }

private:
  std::shared_ptr<Texture> inputBuffer_;
};

} // namespace blkhurst
