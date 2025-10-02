#pragma once
#include <blkhurst/materials/material.hpp>
#include <blkhurst/textures/cube_texture.hpp>
#include <utility>

namespace blkhurst {

struct PrefilterGGXMaterialDesc {
  float lodBias = 2.0F;
  int ggxSamples = 1024;
};

class PrefilterGGXMaterial : public Material {
public:
  PrefilterGGXMaterial(std::shared_ptr<CubeTexture> env, PrefilterGGXMaterialDesc desc)
      : Material(
            Program::createFromRegistry({.vert = "fullscreen_vert", .frag = "prefilter_ggx_frag"})),
        env_(std::move(env)),
        desc_(desc) {
  }

  static std::shared_ptr<PrefilterGGXMaterial> create(std::shared_ptr<CubeTexture> env,
                                                      const PrefilterGGXMaterialDesc& desc) {
    return std::make_shared<PrefilterGGXMaterial>(env, desc);
  }

  void setFace(int face) {
    face_ = face;
  }

  void setRoughness(float roughness) {
    roughness_ = roughness;
  }

  void setGgxSamples(int n) {
    desc_.ggxSamples = n;
  }

  void setLodBias(float bias) {
    desc_.lodBias = bias;
  }

protected:
  void applyResources() override {
    setUniform("uFace", face_);
    setUniform("uRoughness", roughness_);
    setUniform("uGgxSamples", desc_.ggxSamples);
    setUniform("uLodBias", desc_.lodBias);

    bindTextureUnit(env_, "uEnvMap", 0);
  }

private:
  std::shared_ptr<CubeTexture> env_;
  PrefilterGGXMaterialDesc desc_;

  int face_ = 0;
  float roughness_ = 0.0F;
};

} // namespace blkhurst
