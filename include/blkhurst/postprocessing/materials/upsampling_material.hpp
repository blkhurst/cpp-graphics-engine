#pragma once
#include <blkhurst/materials/material.hpp>
#include <blkhurst/materials/uniforms.hpp>
#include <blkhurst/postprocessing/shaders/convolution_upsampling.glsl.hpp>

#include <glm/fwd.hpp>

namespace blkhurst {

class UpsamplingMaterial final : public Material {
public:
  UpsamplingMaterial()
      : Material(Program::create({.vert = shaders::convolution_upsampling_vert,
                                  .frag = shaders::convolution_upsampling_frag})) {
    setDepthWrite(false);
    setDepthTest(false);
  }

  static std::shared_ptr<UpsamplingMaterial> create() {
    return std::make_shared<UpsamplingMaterial>();
  }

  void setRadius(float radius) {
    radius_ = radius;
  }

  void setInputSize(int width, int height) {
    texelSize_ = glm::vec2(1.0F / static_cast<float>(width), 1.0F / static_cast<float>(height));
  }

  void setInputBuffer(const std::shared_ptr<Texture>& texture) {
    inputBuffer_ = texture;
  }

  void setSupportBuffer(const std::shared_ptr<Texture>& texture) {
    supportBuffer_ = texture;
  }

protected:
  void applyResources() override {
    setUniform("uTexelSize", texelSize_);
    setUniform("uRadius", radius_);
    bindTextureUnit(inputBuffer_, samplers::InputBuffer, slots::Postprocessing0);
    bindTextureUnit(supportBuffer_, "uSupportBuffer", slots::Postprocessing1);
  }

private:
  std::shared_ptr<Texture> inputBuffer_;
  std::shared_ptr<Texture> supportBuffer_;
  glm::vec2 texelSize_{1.0F, 1.0F};
  float radius_ = 1.0F; // ~[0.7, 0.85]
};

} // namespace blkhurst
