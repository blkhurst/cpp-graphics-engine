#include <blkhurst/postprocessing/materials/copy_material.hpp>
#include <blkhurst/postprocessing/passes/output_pass.hpp>

namespace blkhurst {

OutputPass::OutputPass()
    : copyMaterial_(CopyMaterial::create()),
      copyQuad_(copyMaterial_) {
  setNeedsSwap(false);
}

std::shared_ptr<OutputPass> OutputPass::create() {
  return std::make_shared<OutputPass>();
}

void OutputPass::render(const PassRenderContext& context) {
  if (!isEnabled() || !copyMaterial_) {
    return;
  }

  // Set ReadBuffer Texture
  copyMaterial_->setInputBuffer(context.readBuffer->texture());

  // Set RenderTarget
  context.renderToScreen ? context.renderer->setRenderTarget(nullptr)
                         : context.renderer->setRenderTarget(context.writeBuffer);

  // Fullscreen draw
  copyQuad_.render(*context.renderer);
}

} // namespace blkhurst
