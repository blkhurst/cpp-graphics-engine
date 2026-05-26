#include <blkhurst/postprocessing/passes/vignette_pass.hpp>
#include <blkhurst/renderer/renderer.hpp>

#include <algorithm>

namespace blkhurst {

VignettePass::VignettePass(const VignettePassDesc& desc)
    : desc_(desc) {
  setTechnique(desc_.technique);
  setOffset(desc_.offset);
  setDarkness(desc_.darkness);
}

std::shared_ptr<VignettePass> VignettePass::create(const VignettePassDesc& desc) {
  return std::make_shared<VignettePass>(desc);
}

const VignettePassDesc& VignettePass::desc() const {
  return desc_;
}

void VignettePass::setTechnique(VignetteTechnique technique) {
  desc_.technique = technique;
  vignetteMaterial_->setTechnique(desc_.technique);
}

void VignettePass::setOffset(float offset) {
  desc_.offset = std::max(0.0F, offset);
  vignetteMaterial_->setOffset(desc_.offset);
}

void VignettePass::setDarkness(float darkness) {
  desc_.darkness = std::max(0.0F, darkness);
  vignetteMaterial_->setDarkness(desc_.darkness);
}

void VignettePass::render(const PassRenderContext& context) {
  if (!isEnabled()) {
    return;
  }

  vignetteMaterial_->setInputBuffer(context.readBuffer->texture());

  context.renderToScreen ? context.renderer->setRenderTarget(nullptr)
                         : context.renderer->setRenderTarget(context.writeBuffer);

  vignetteQuad_.render(*context.renderer);
}

} // namespace blkhurst
