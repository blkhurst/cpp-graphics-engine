#include <blkhurst/postprocessing/passes/render_pass.hpp>
#include <blkhurst/renderer/renderer.hpp>

namespace blkhurst {

RenderPass::RenderPass(Object3D* root, Camera* camera)
    : root_(root),
      camera_(camera) {
}

std::shared_ptr<RenderPass> RenderPass::create(Object3D* root, Camera* camera) {
  return std::make_shared<RenderPass>(root, camera);
}

void RenderPass::setScene(Object3D* root) {
  root_ = root;
}

void RenderPass::setCamera(Camera* camera) {
  camera_ = camera;
}

void RenderPass::render(const PassRenderContext& context) {
  if (!isEnabled() || root_ == nullptr || camera_ == nullptr) {
    return;
  }

  // Select RenderTarget
  context.renderToScreen ? context.renderer->setRenderTarget(nullptr)
                         : context.renderer->setRenderTarget(context.writeBuffer);

  // Render Scene
  context.renderer->render(*root_, *camera_);
}

} // namespace blkhurst
