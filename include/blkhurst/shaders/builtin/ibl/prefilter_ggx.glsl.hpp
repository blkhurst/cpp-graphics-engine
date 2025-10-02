#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string prefilter_ggx_frag = R"GLSL(

#include "common"
#include "io_fragment"
#include "uniforms_common"

#include "pbr_common"

uniform int uFace;
uniform float uRoughness;
uniform int uGgxSamples;
uniform float uLodBias;
uniform samplerCube uEnvMap;

const float MIN_PDF = 1e-4;

void main() {
  //*
  // Query uEnvMap size and mip levels
  float faceRes = float(textureSize(uEnvMap, 0).x);
  float maxLod = float(textureQueryLevels(uEnvMap) - 1); // floor(log2(faceRes))

  // Learnopengl Chetan Jags method reduces but does not prevent artifacts.
  // This tunable bias can be tweaked to prevent bright dots that may appear
  // when sampling a HDR with strong pin-point light sources. Typically [0..2]
  float lodBias = uLodBias * uRoughness; // Issue increases with roughness, hence higher bias
  //*

  vec3 N = faceToDirection(uFace, vUv);
  vec3 V = N;

  const uint SAMPLE_COUNT = uint(uGgxSamples); //1024u;
  vec3 prefilteredColor = vec3(0.0);
  float totalWeight = 0.0;

  for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
    vec2 Xi = Hammersley(i, SAMPLE_COUNT);

    vec3 H = ImportanceSampleGGX(Xi, N, uRoughness);
    float HdotV = max(dot(H, V), 0.0);
    if (HdotV <= 0.0) continue;

    vec3 L = normalize(2.0 * HdotV * H - V);
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) continue;

    // sample from the environment's mip level based on roughness/pdf
    float D = DistributionGGX(N, H, uRoughness);
    float NdotH = max(dot(N, H), 0.0);
    float pdf = max(MIN_PDF, (D * NdotH) / (4.0 * HdotV));

    float saTexel = 4.0 * PI / (6.0 * faceRes * faceRes);
    float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf);

    float mipLevel = (uRoughness <= 0.0) ? 0.0 : 0.5 * log2(saSample / saTexel);
    mipLevel = clamp(mipLevel + lodBias, 0.0, maxLod); //* Added to bias towards blurrier mips

    prefilteredColor += textureLod(uEnvMap, L, mipLevel).rgb * NdotL;
    totalWeight += NdotL;
  }

  prefilteredColor /= max(totalWeight, 1e-5);

  FragColor = vec4(prefilteredColor, 1.0);
}

)GLSL";

} // namespace blkhurst::shaders
