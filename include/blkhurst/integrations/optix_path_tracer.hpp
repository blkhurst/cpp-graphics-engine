#pragma once

#ifdef BLKHURST_ENABLE_OPTIX

#include <memory>

namespace blkhurst {

class Camera;
class Object3D;
class Texture;

enum class OptixIntegratorMode : int {
  FirstHit = 0,
  PathTracing = 1,
  DirectLighting = 2,
  Mis = 3,
};

enum class OptixSamplingMode : int {
  UniformHemisphere = 0,
  CosineHemisphere = 1,
  Ggx = 2,
  GgxVndf = 3,
};

enum class OptixEnvironmentMode : int {
  Off = 0,
  Flat = 1,
  Hdri = 2,
};

enum class OptixMaterialMode : int {
  Lambert = 0,
  PbrGgx = 1,
};

enum class OptixMisMode : int {
  Off = 0,
  Balance = 1,
  Power = 2,
};

enum class OptixDebugView : int {
  Beauty = 0,
  Normals = 1,
  Albedo = 2,
  Depth = 3,
  Direct = 4,
  Indirect = 5,
  Pdf = 6,
  MisWeight = 7,
  BounceCount = 8,
};

struct OptixPathTracerDesc {
  int samplesPerPixel = 1;
  int maxBounces = 5;
  OptixIntegratorMode integratorMode = OptixIntegratorMode::Mis;
  OptixSamplingMode samplingMode = OptixSamplingMode::GgxVndf;
  OptixEnvironmentMode environmentMode = OptixEnvironmentMode::Hdri;
  OptixMaterialMode materialMode = OptixMaterialMode::PbrGgx;
  OptixMisMode misMode = OptixMisMode::Power;
  OptixDebugView debugView = OptixDebugView::Beauty;
  bool enableTextures = true;
  bool enableNormalMaps = true;
  bool enableAlpha = true;
  bool enableMirrorReflection = true;
  bool enableDirectLighting = true;
  bool enableShadowRays = true;
  bool enableEmissiveLights = true;
  bool enableRussianRoulette = false;
};

class OptixPathTracer {
public:
  explicit OptixPathTracer(const OptixPathTracerDesc& desc = {});
  ~OptixPathTracer();

  OptixPathTracer(const OptixPathTracer&) = delete;
  OptixPathTracer& operator=(const OptixPathTracer&) = delete;
  OptixPathTracer(OptixPathTracer&&) = delete;
  OptixPathTracer& operator=(OptixPathTracer&&) = delete;

  void setScene(Object3D* scene);
  void setCamera(Camera* camera);
  void setSize(int width, int height);
  void setSamplesPerPixel(int samplesPerPixel);
  void setMaxBounces(int maxBounces);
  void setDesc(const OptixPathTracerDesc& desc);
  void setAccumulate(bool accumulate);
  void resetAccumulation();
  void renderTo(Texture& output);

private:
  struct Impl;
  Impl* impl_ = nullptr;
};

} // namespace blkhurst

#endif // BLKHURST_ENABLE_OPTIX
