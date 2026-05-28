#include <blkhurst/integrations/optix_path_trace_pass.hpp>

#ifdef BLKHURST_ENABLE_OPTIX

#include <blkhurst/cameras/camera.hpp>
#include <blkhurst/renderer/render_target.hpp>
#include <blkhurst/textures/texture.hpp>

#include <spdlog/spdlog.h>

#include <exception>

namespace blkhurst {

glm::mat4 previousView_{1.0F};
glm::mat4 previousProjection_{1.0F};

bool cameraChanged(const Camera& camera) {
  constexpr float epsilon = 0.00001F;
  const glm::mat4 viewDiff = camera.viewMatrix() - previousView_;
  const glm::mat4 projectionDiff = camera.projectionMatrix() - previousProjection_;
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      if (std::abs(viewDiff[col][row]) > epsilon || std::abs(projectionDiff[col][row]) > epsilon) {
        return true;
      }
    }
  }
  return false;
}

OptixPathTracePass::OptixPathTracePass(Object3D* scene, Camera* camera,
                                       const OptixPathTracerDesc& desc) {
  scene_ = scene;
  camera_ = camera;
  try {
    tracer_ = std::make_unique<OptixPathTracer>(desc);
    tracer_->setScene(scene_);
    tracer_->setCamera(camera_);
  } catch (const std::exception& e) {
    spdlog::error("OptixPathTracePass disabled: {}", e.what());
    setEnabled(false);
  }
}

OptixPathTracePass::~OptixPathTracePass() = default;

std::shared_ptr<OptixPathTracePass> OptixPathTracePass::create(Object3D* scene, Camera* camera,
                                                               const OptixPathTracerDesc& desc) {
  return std::make_shared<OptixPathTracePass>(scene, camera, desc);
}

void OptixPathTracePass::setScene(Object3D* scene) {
  scene_ = scene;
  if (tracer_) {
    tracer_->setScene(scene);
  }
}

void OptixPathTracePass::setCamera(Camera* camera) {
  camera_ = camera;
  if (tracer_) {
    tracer_->setCamera(camera);
  }
}

void OptixPathTracePass::setSamplesPerPixel(int samplesPerPixel) {
  if (tracer_) {
    tracer_->setSamplesPerPixel(samplesPerPixel);
  }
}

void OptixPathTracePass::setMaxBounces(int maxBounces) {
  if (tracer_) {
    tracer_->setMaxBounces(maxBounces);
  }
}

void OptixPathTracePass::setDesc(const OptixPathTracerDesc& desc) {
  if (tracer_) {
    tracer_->setDesc(desc);
  }
}

void OptixPathTracePass::setAccumulate(bool accumulate) {
  if (tracer_) {
    tracer_->setAccumulate(accumulate);
  }
}

void OptixPathTracePass::resetAccumulation() {
  if (tracer_) {
    tracer_->resetAccumulation();
  }
}

void OptixPathTracePass::setSize(int width, int height) {
  Pass::setSize(width, height);
  if (tracer_) {
    tracer_->setSize(width, height);
  }
}

void OptixPathTracePass::render(const PassRenderContext& context) {
  if (!isEnabled() || !tracer_ || context.writeBuffer == nullptr ||
      !context.writeBuffer->texture()) {
    return;
  }
  if (context.renderToScreen) {
    spdlog::warn(
        "OptixPathTracePass requires a following OutputPass to present the traced result.");
    return;
  }

  if (camera_ && tracer_ && cameraChanged(*camera_)) {
    tracer_->resetAccumulation();
    previousView_ = camera_->viewMatrix();
    previousProjection_ = camera_->projectionMatrix();
  }

  try {
    tracer_->renderTo(*context.writeBuffer->texture());
  } catch (const std::exception& e) {
    spdlog::error("OptixPathTracePass disabled: {}", e.what());
    setEnabled(false);
  }
}

} // namespace blkhurst

#endif // BLKHURST_ENABLE_OPTIX
