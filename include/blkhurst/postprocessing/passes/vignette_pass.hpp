#pragma once
#include <blkhurst/helpers/fullscreen_quad.hpp>
#include <blkhurst/postprocessing/materials/vignette_material.hpp>
#include <blkhurst/postprocessing/pass.hpp>

#include <memory>

/**
Vignette Pass
Fullscreen vignette pass based on pmndrs/postprocessing's VignetteEffect.
*/

namespace blkhurst {

struct VignettePassDesc {
  VignetteTechnique technique = VignetteTechnique::Default;
  float offset = 0.5F;
  float darkness = 0.5F;
};

class VignettePass : public Pass {
public:
  VignettePass(const VignettePassDesc& desc = {});
  static std::shared_ptr<VignettePass> create(const VignettePassDesc& desc = {});

  [[nodiscard]] const VignettePassDesc& desc() const;

  void setTechnique(VignetteTechnique technique);
  void setOffset(float offset);
  void setDarkness(float darkness);

  void render(const PassRenderContext& context) override;

private:
  VignettePassDesc desc_;

  std::shared_ptr<VignetteMaterial> vignetteMaterial_ = VignetteMaterial::create();
  FullscreenQuad vignetteQuad_{vignetteMaterial_};
};

} // namespace blkhurst
