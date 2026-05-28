#pragma once

#ifdef BLKHURST_ENABLE_OPTIX

#include <blkhurst/integrations/optix_path_tracer.hpp>
#include <blkhurst/postprocessing/pass.hpp>

#include <memory>

namespace blkhurst {

class Camera;
class Object3D;

class OptixPathTracePass : public Pass {
public:
  OptixPathTracePass(Object3D* scene, Camera* camera, const OptixPathTracerDesc& desc = {});
  ~OptixPathTracePass() override;

  static std::shared_ptr<OptixPathTracePass> create(Object3D* scene, Camera* camera,
                                                    const OptixPathTracerDesc& desc = {});

  void setScene(Object3D* scene);
  void setCamera(Camera* camera);
  void setSamplesPerPixel(int samplesPerPixel);
  void setMaxBounces(int maxBounces);
  void setDesc(const OptixPathTracerDesc& desc);
  void setAccumulate(bool accumulate);
  void resetAccumulation();
  void setSize(int width, int height) override;
  void render(const PassRenderContext& context) override;

private:
  std::unique_ptr<OptixPathTracer> tracer_;
  Object3D* scene_ = nullptr;
  Camera* camera_ = nullptr;
};

} // namespace blkhurst

#endif // BLKHURST_ENABLE_OPTIX
