#pragma once
#include <blkhurst/materials/material.hpp>
#include <blkhurst/materials/uniforms.hpp>
#include <blkhurst/postprocessing/shaders/vignette.glsl.hpp>
#include <blkhurst/shaders/builtin/fullscreen.glsl.hpp>
#include <blkhurst/textures/texture.hpp>

#include <memory>

namespace blkhurst {

enum class VignetteTechnique : int { Default = 0, Eskil = 1 };

class VignetteMaterial final : public Material {
public:
  VignetteMaterial()
      : Material(
            Program::create({.vert = shaders::fullscreen_vert, .frag = shaders::vignette_frag})) {
    setDepthWrite(false);
    setDepthTest(false);
  }

  static std::shared_ptr<VignetteMaterial> create() {
    return std::make_shared<VignetteMaterial>();
  }

  void setInputBuffer(const std::shared_ptr<Texture>& texture) {
    inputBuffer_ = texture;
  }

  void setTechnique(VignetteTechnique technique) {
    technique_ = technique;
  }

  void setOffset(float offset) {
    offset_ = offset;
  }

  void setDarkness(float darkness) {
    darkness_ = darkness;
  }

protected:
  void applyResources() override {
    setUniform("uTechnique", static_cast<int>(technique_));
    setUniform("uOffset", offset_);
    setUniform("uDarkness", darkness_);
    bindTextureUnit(inputBuffer_, samplers::InputBuffer, slots::Postprocessing0);
  }

private:
  std::shared_ptr<Texture> inputBuffer_;
  VignetteTechnique technique_ = VignetteTechnique::Default;
  float offset_ = 0.5F;
  float darkness_ = 0.5F;
};

} // namespace blkhurst
