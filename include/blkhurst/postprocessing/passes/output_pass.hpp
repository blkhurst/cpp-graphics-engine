#pragma once
#include <blkhurst/helpers/fullscreen_quad.hpp>
#include <blkhurst/postprocessing/pass.hpp>

#include <memory>

namespace blkhurst {

class CopyMaterial;

class OutputPass : public Pass {
public:
  OutputPass();
  static std::shared_ptr<OutputPass> create();

  void render(const PassRenderContext& context) override;

private:
  // Initialisation Order Matters
  std::shared_ptr<CopyMaterial> copyMaterial_;
  FullscreenQuad copyQuad_;
};

} // namespace blkhurst
