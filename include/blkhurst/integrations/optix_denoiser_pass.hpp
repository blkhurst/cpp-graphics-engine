#pragma once

#ifdef BLKHURST_ENABLE_OPTIX

#include <blkhurst/postprocessing/pass.hpp>

#include <memory>

namespace blkhurst {

struct OptixDenoiserPassDesc {
  bool useAlbedo = false;
  bool useNormal = false;
};

class OptixDenoiserPass : public Pass {
public:
  explicit OptixDenoiserPass(const OptixDenoiserPassDesc& desc);
  ~OptixDenoiserPass() override;

  OptixDenoiserPass(const OptixDenoiserPass&) = delete;
  OptixDenoiserPass& operator=(const OptixDenoiserPass&) = delete;
  OptixDenoiserPass(OptixDenoiserPass&&) = delete;
  OptixDenoiserPass& operator=(OptixDenoiserPass&&) = delete;

  static std::shared_ptr<OptixDenoiserPass> create(const OptixDenoiserPassDesc& desc = {});

  void setSize(int width, int height) override;
  void render(const PassRenderContext& context) override;

private:
  struct Impl;
  Impl* impl_ = nullptr;
};

} // namespace blkhurst

#endif // BLKHURST_ENABLE_OPTIX
