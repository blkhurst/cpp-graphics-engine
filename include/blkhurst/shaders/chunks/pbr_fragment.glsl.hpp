#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string pbr_fragment = R"GLSL(

// *Depends on
//  common
//    PI
//  io_fragment
//    in vec2 vUv
//    in vec3 vWorldPosition
//  lights_common
//    uniform LightData uLightData
//    buffer DirectionalLights uDirectionalLights;
//    buffer PointLights uPointLights;
//  pbr_common
//    DistributionGGX
//    GeometrySmith_Direct
//    fresnelSchlick

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

struct Surface {
  vec3 N;
  vec3 V;
  vec3 R;
  vec3 albedo;
  vec3 F0;
  float roughness;
  float metalness;
  float ao;
};

struct LightSample {
  vec3 L;        // fragment -> light (incoming dir)
  float NdotL;   // max(dot(N, L), 0)
  vec3 radiance; // light color * intensity * attenuation
};

vec3 evaluateBRDF(const Surface s, const LightSample ls) {
  vec3 H = normalize(s.V + ls.L);
  float NDF = DistributionGGX(s.N, H, s.roughness);
  float G = GeometrySmith_Direct(s.N, s.V, ls.L, s.roughness);
  vec3 F = fresnelSchlick(max(dot(H, s.V), 0.0), s.F0);

  vec3 nominator = NDF * G * F;
  float denominator = 4.0 * max(dot(s.N, s.V), 0.0) * ls.NdotL + 0.0001;
  vec3 specular = nominator / denominator;

  // kS is equal to Fresnel
  vec3 kS = F;
  // for energy conservation, the diffuse and specular light can't
  // be above 1.0 (unless the surface emits light); to preserve this
  // relationship the diffuse component (kD) should equal 1.0 - kS.
  vec3 kD = vec3(1.0) - kS;
  // multiply kD by the inverse metalness such that only non-metals
  // have diffuse lighting, or a linear blend if partly metal (pure metals
  // have no diffuse light).
  kD *= (1.0 - s.metalness);

  // scale light by NdotL
  // float NdotL = max(dot(N, L), 0.0);

  // add to outgoing Lo
  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
  return (kD * s.albedo / PI + specular) * ls.radiance * ls.NdotL;
}

vec3 accumulateLights(vec3 N, vec3 V, vec3 R, vec3 F0, vec3 albedo, float metalness,
                      float roughness, float ao) {
  Surface s;
  s.N = N;
  s.V = V;
  s.R = R;
  s.albedo = albedo;
  s.F0 = F0;
  s.roughness = max(roughness, 0.0525); // 0.0525 corresponds to the base mip of a 256 cubemap.
  s.metalness = metalness;

  vec3 Lo = vec3(0.0);

  // Ambient Light
  float kD = 1.0 - metalness;
  vec3 lambert = albedo / PI;
  vec3 irradiance = uLightData.ambientColor * uLightData.ambientIntensity * ao; //? THIS?
  Lo += kD * lambert * irradiance;

  // Point Lights
  for (int i = 0; i < uLightData.pointCount; ++i) {
    PointLightGPU light = uPointLights.uPointLights[i];

    vec3 lVector = light.worldPosition - vWorldPosition;
    vec3 L = normalize(lVector);
    float lightDistance = length(lVector);
    float attenuation = getDistanceAttenuation(lightDistance, light.distance, light.decay);

    LightSample ls;
    ls.L = L;
    ls.NdotL = max(dot(N, ls.L), 0.0);
    ls.radiance = light.color * light.intensity * attenuation;

    Lo += evaluateBRDF(s, ls);
  }

  // Directional Lights
  for (int i = 0; i < uLightData.directionalCount; ++i) {
    DirectionalLightGPU light = uDirectionalLights.uDirectionalLights[i];

    LightSample ls;
    ls.L = normalize(-light.worldDirection); // Direction light is incoming from
    ls.NdotL = max(dot(N, ls.L), 0.0);
    ls.radiance = light.color * light.intensity;

    Lo += evaluateBRDF(s, ls);
  }

  return Lo;
}

)GLSL";

} // namespace blkhurst::shaders
