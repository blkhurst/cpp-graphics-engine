#pragma once
#include "blkhurst/materials/uniforms.hpp"
#include <blkhurst/materials/material.hpp>
#include <blkhurst/postprocessing/shaders/bloom_mix.glsl.hpp>
#include <blkhurst/shaders/builtin/fullscreen.glsl.hpp>
#include <blkhurst/textures/texture.hpp>

#include <memory>

namespace blkhurst {

class BloomMixMaterial final : public Material {
public:
  BloomMixMaterial()
      : Material(
            Program::create({.vert = shaders::fullscreen_vert, .frag = shaders::bloom_mix_frag})) {
    setDepthTest(false);
    setDepthWrite(false);
  }

  static std::shared_ptr<BloomMixMaterial> create() {
    return std::make_shared<BloomMixMaterial>();
  };

  void setInputBuffer(const std::shared_ptr<Texture>& texture) {
    inputBuffer_ = texture;
  }

  void setBloomBuffer(const std::shared_ptr<Texture>& texture) {
    bloomBuffer_ = texture;
  }

  void setBloomIntensity(float intensity) {
    bloomIntensity_ = intensity;
  }

protected:
  void applyResources() override {
    setUniform("uBloomIntensity", bloomIntensity_);
    bindTextureUnit(inputBuffer_, samplers::InputBuffer, slots::Postprocessing0);
    bindTextureUnit(bloomBuffer_, "uBloomBuffer", slots::Postprocessing1);
  }

private:
  std::shared_ptr<Texture> inputBuffer_;
  std::shared_ptr<Texture> bloomBuffer_;
  float bloomIntensity_ = 0.1F;
};

} // namespace blkhurst
