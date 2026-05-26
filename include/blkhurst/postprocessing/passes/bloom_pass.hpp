#pragma once
#include <blkhurst/helpers/fullscreen_quad.hpp>
#include <blkhurst/postprocessing/materials/bloom_mix_material.hpp>
#include <blkhurst/postprocessing/materials/downsampling_material.hpp>
#include <blkhurst/postprocessing/materials/upsampling_material.hpp>
#include <blkhurst/postprocessing/pass.hpp>

#include <memory>
#include <vector>

/**
Bloom - 13-Tap Downsample 9-Tap Upsample + Lerp

This bloom implementation follows the method described by Froyok.
- Uses HDR brightness instead of a luminosity threshold.
- Downsamples using a CoD-style 13-tap kernel with Froyok's weights.
- Upsamples using a CoD-style 9-tap 3x3 tent-filter, blending with linear interpolation.
- Final composite again uses linear interpolation, not additive blending.

radius    - Internal upsample lerp/blend factor
intensity - Final mix strength. Typically <= 0.3 to avoid softening the scene;
            for stronger bloom, increase material emissive intensity instead.
*/

namespace blkhurst {

struct BloomPassDesc {
  int downsampleLevels = 8;
  int upsampleLevels = 7; // Must be < downsampleLevels (Nothing to sample from at highest level)
  float radius = 0.85F;   // internal upsample lerp (shape)
  float intensity = 0.1F; // Mix Strength (Typically <= 0.3)
};

class BloomPass : public Pass {
public:
  BloomPass(const BloomPassDesc& desc = {});
  static std::shared_ptr<BloomPass> create(const BloomPassDesc& desc = {});

  [[nodiscard]] const BloomPassDesc& desc() const;

  void setDownsamplingLevels(int levels);
  void setUpsamplingLevels(int levels);
  void setRadius(float radius);
  void setIntensity(float intensity);

  void setSize(int width, int height) override;
  void render(const PassRenderContext& context) override;

private:
  BloomPassDesc desc_;
  glm::ivec2 resolution_{1, 1};

  // Custom Mipmaps
  std::vector<std::shared_ptr<RenderTarget>> mipDownsamples_;
  std::vector<std::shared_ptr<RenderTarget>> mipUpsamples_;

  // 13 Tap Downsample
  std::shared_ptr<DownsamplingMaterial> downsampleMat_ = DownsamplingMaterial::create();
  FullscreenQuad downsampleQuad_{downsampleMat_};

  // 9 Tap Upsample+Lerp
  std::shared_ptr<UpsamplingMaterial> upsampleMat_ = UpsamplingMaterial::create();
  FullscreenQuad upsampleQuad_{upsampleMat_};

  // Final Mix
  std::shared_ptr<BloomMixMaterial> mixMaterial_ = BloomMixMaterial::create();
  FullscreenQuad mixQuad_{mixMaterial_};
};

} // namespace blkhurst
