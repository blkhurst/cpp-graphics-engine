#pragma once

#ifdef BLKHURST_ENABLE_OPTIX

#include <blkhurst/integrations/optix_denoiser_pass.hpp>
#include <blkhurst/integrations/optix_path_trace_pass.hpp>
#include <blkhurst/postprocessing/effect_composer.hpp>
#include <blkhurst/postprocessing/passes/output_pass.hpp>
#include <blkhurst/postprocessing/passes/render_pass.hpp>

#include <memory>

namespace blkhurst {

class Camera;
class Object3D;

class OptixPass {
public:
  OptixPass(Object3D* scene, Camera* camera, const OptixPathTracerDesc& desc = {})
      : renderPass_(RenderPass::create(scene, camera)),
        pathTracePass_(OptixPathTracePass::create(scene, camera, desc)),
        denoiserPass_(OptixDenoiserPass::create()),
        outputPass_(OutputPass::create()),
        desc_(desc) {
    apply();
  }

  static std::shared_ptr<OptixPass> create(Object3D* scene, Camera* camera,
                                           const OptixPathTracerDesc& desc = {}) {
    return std::make_shared<OptixPass>(scene, camera, desc);
  }

  void addTo(EffectComposer& composer) const {
    composer.addPass(renderPass_);
    composer.addPass(pathTracePass_);
    composer.addPass(denoiserPass_);
    composer.addPass(outputPass_);
  }

  void setUsePathTracing(bool enabled) {
    usePathTracing_ = enabled;
    apply();
  }

  void setDenoise(bool enabled) {
    denoise_ = enabled;
    apply();
  }

  void setAccumulate(bool enabled) {
    accumulate_ = enabled;
    apply();
  }

  void setSamplesPerPixel(int samplesPerPixel) {
    desc_.samplesPerPixel = samplesPerPixel;
    apply();
  }

  void setMaxBounces(int maxBounces) {
    desc_.maxBounces = maxBounces;
    apply();
  }

  void setPathTracerDesc(const OptixPathTracerDesc& desc) {
    desc_ = desc;
    apply();
  }

  void resetAccumulation() const {
    if (pathTracePass_) {
      pathTracePass_->resetAccumulation();
    }
  }

  [[nodiscard]] bool usePathTracing() const {
    return usePathTracing_;
  }

  [[nodiscard]] bool denoise() const {
    return denoise_;
  }

  [[nodiscard]] bool accumulate() const {
    return accumulate_;
  }

  [[nodiscard]] int samplesPerPixel() const {
    return desc_.samplesPerPixel;
  }

  [[nodiscard]] int maxBounces() const {
    return desc_.maxBounces;
  }

  [[nodiscard]] const OptixPathTracerDesc& pathTracerDesc() const {
    return desc_;
  }

  [[nodiscard]] const std::shared_ptr<OptixPathTracePass>& pathTracePass() const {
    return pathTracePass_;
  }

  [[nodiscard]] const std::shared_ptr<OptixDenoiserPass>& denoiserPass() const {
    return denoiserPass_;
  }

private:
  std::shared_ptr<RenderPass> renderPass_;
  std::shared_ptr<OptixPathTracePass> pathTracePass_;
  std::shared_ptr<OptixDenoiserPass> denoiserPass_;
  std::shared_ptr<OutputPass> outputPass_;

  bool usePathTracing_ = true;
  bool denoise_ = true;
  bool accumulate_ = true;
  OptixPathTracerDesc desc_{};

  void apply() {
    if (renderPass_) {
      renderPass_->setEnabled(!usePathTracing_);
    }
    if (pathTracePass_) {
      pathTracePass_->setEnabled(usePathTracing_);
      pathTracePass_->setAccumulate(accumulate_);
      pathTracePass_->setDesc(desc_);
    }
    if (denoiserPass_) {
      denoiserPass_->setEnabled(usePathTracing_ && denoise_);
    }
  }
};

} // namespace blkhurst

#endif // BLKHURST_ENABLE_OPTIX
