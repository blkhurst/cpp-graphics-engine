#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string pbr_vert = R"GLSL(

#include "io_vertex"
#include "uniforms_common"

void main() {
  io_vertex(uModel, uView, uProjection);
}

)GLSL";

inline const std::string pbr_frag = R"GLSL(

#include "common"
#include "io_fragment"
#include "uniforms_common"
#include "normal_fragment"
#include "color_fragment"
#include "envmap_fragment"
#include "tonemapping_fragment"
#include "colorspace_fragment"

#include "pbr_common"
#include "ibl_fragment"

// Material parameters
// uniform sampler2D uColorMap; // color_fragment
// uniform sampler2D uAlphaMap; // color_fragment
// uniform sampler2D uNormalMap; // normal_fragment
uniform sampler2D uMetalnessMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uAoMap;
uniform sampler2D uEmissiveMap;

uniform float uMetalness;
uniform float uRoughness;
uniform float uAoIntensity;
uniform vec3 uEmissiveColor;
uniform float uEmissiveIntensity;

vec3 computeF0(vec3 albedo, float metalness) {
  // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
  // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
  vec3 F0 = vec3(0.04);
  return mix(F0, albedo, metalness);
}

float computeMetalness() {
  float metalness = uMetalness;
#ifdef USE_METALNESSMAP
  //* Supports OcclusionRoughnessMetalness (RGB) packing
  metalness *= texture(uMetalnessMap, vUv).b; // Reads channel B
#endif
  return metalness;
}

float computeRoughness() {
  float roughness = uRoughness;
#ifdef USE_ROUGHNESSMAP
  roughness *= texture(uRoughnessMap, vUv).g; // Reads channel G
#endif
  return roughness;
}

float computeAo() {
  float ao = 1.0;
#ifdef USE_AOMAP
  float aoMap = texture(uAoMap, vUv).r; // Reads channel R
  ao = mix(1.0, aoMap, uAoIntensity);
#endif
  return ao;
}

vec3 computeEmissive() {
  vec3 emissive = uEmissiveColor * uEmissiveIntensity;
#ifdef USE_EMISSIVEMAP
  emissive *= texture(uEmissiveMap, vUv).rgb;
#endif
  return emissive;
}

void main() {
  vec3 albedo = computeColor();
  float alpha = computeAlpha();
  float metalness = computeMetalness();
  float roughness = computeRoughness();
  float ao = computeAo();
  vec3 emission = computeEmissive();

  vec3 N = computeWorldNormal();
  vec3 V = normalize(uCameraPos - vWorldPosition);
  vec3 R = reflect(-V, N);

  vec3 F0 = computeF0(albedo, metalness);

  vec3 Lo = vec3(0.0);
  // TODO: Accumulate Lights..

  vec3 ibl = computeIBL(N, V, R, F0, albedo, metalness, roughness);
  vec3 ambient = ibl * ao;

  vec4 color = vec4(ambient + Lo + emission, alpha);

  // Tone Mapping + Color Space
  vec4 toneMapped = toneMapping(color);
  FragColor = linearToOutput(toneMapped);
}

)GLSL";

} // namespace blkhurst::shaders
