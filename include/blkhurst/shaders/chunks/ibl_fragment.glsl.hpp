#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string ibl_fragment = R"GLSL(

// *Depends on:
//  common
//    getCubeSampleDir
//  pbr_fragment
//    fresnelSchlickRoughness

uniform sampler2D uBrdfLUT;
uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilterMap;

uniform mat3 uEnvRotation;
uniform float uEnvIntensity;

vec3 getIBLIrradiance(vec3 N) {
  vec3 dir = getCubeSampleDir(N, uEnvRotation);
  vec3 irradiance = textureLod(uIrradianceMap, dir, 0.0).rgb; // Force LOD 0
  return irradiance * uEnvIntensity;
}

vec3 getIBLRadiance(vec3 R, float roughness) {
  vec3 dir = getCubeSampleDir(R, uEnvRotation);

  // Compute LOD for prefiltered environment map (LOD_MIN 4 16x16)
  float prefilterMaxLod = float(textureQueryLevels(uPrefilterMap) - 1);
  float lod = clamp(roughness * prefilterMaxLod, 0.0, prefilterMaxLod);

  vec3 prefilteredColor = textureLod(uPrefilterMap, dir, lod).rgb;
  return prefilteredColor * uEnvIntensity;
}

vec3 computeIBL(vec3 N, vec3 V, vec3 R, vec3 F0, vec3 albedo, float metalness, float roughness) {
  //* Note:
  // Early exits via uniforms/variables do not prevent NaN/disappearance when sampler unbound.
  // Fixes include guarding texture/textureLod with ifdef,
  // or ifndef early return (compiler optimises null code if guaranteed to return early)
  #ifndef USE_IBL
    return vec3(0.0);
  #endif

  float NdotV = max(dot(N, V), 0.0);

  vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);

  vec3 kS = F;
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metalness;

  vec3 irradiance = getIBLIrradiance(N);
  // vec3 diffuse = (irradiance / PI) * albedo; // SH - Use if irradiance is raw E(N)
  vec3 diffuse = irradiance * albedo; // LearnOpenGL - Use if irradiance bakes 1/PI

  vec3 prefilteredColor = getIBLRadiance(R, roughness);
  vec2 brdf = texture(uBrdfLUT, vec2(NdotV, roughness)).rg;
  vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

  vec3 ambient = (kD * diffuse + specular);
  return ambient;
}

)GLSL";

} // namespace blkhurst::shaders
