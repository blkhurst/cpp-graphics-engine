#pragma once
#include <blkhurst/materials/material.hpp>
#include <blkhurst/materials/uniforms.hpp>
#include <blkhurst/postprocessing/shaders/convolution_downsampling.glsl.hpp>

#include <glm/fwd.hpp>

namespace blkhurst {

class DownsamplingMaterial final : public Material {
public:
  DownsamplingMaterial()
      : Material(Program::create({.vert = shaders::convolution_downsampling_vert,
                                  .frag = shaders::convolution_downsampling_frag})) {
    setDepthWrite(false);
    setDepthTest(false);
  }

  static std::shared_ptr<DownsamplingMaterial> create() {
    return std::make_shared<DownsamplingMaterial>();
  }

  void setInputSize(int width, int height) {
    texelSize_ = glm::vec2(1.0F / static_cast<float>(width), 1.0F / static_cast<float>(height));
  }

  void setInputBuffer(const std::shared_ptr<Texture>& texture) {
    inputBuffer_ = texture;
  }

protected:
  void applyResources() override {
    setUniform("uTexelSize", texelSize_);
    bindTextureUnit(inputBuffer_, samplers::InputBuffer, slots::Postprocessing0);
  }

private:
  std::shared_ptr<Texture> inputBuffer_;
  glm::vec2 texelSize_{1.0F, 1.0F};
};

} // namespace blkhurst
