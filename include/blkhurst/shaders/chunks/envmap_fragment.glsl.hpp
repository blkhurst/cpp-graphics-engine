#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string envmap_fragment = R"GLSL(

// *Depends on:
// io_fragment
//  in vec3 vWorldNormal;
//  in vec3 vWorldPosition;
// uniforms_common
//  uniform vec3 uCameraPos;

uniform samplerCube uEnvMap;
uniform float uReflectivity;
uniform float uRefractionRatio;

// TODO: Flip via uniform
const float flipEnvMap = -1.0;

vec4 computeEnv(in vec3 worldNormal) {
#ifndef USE_ENVMAP
  return vec4(1.0);
#endif

  // View Vector (fragToCamera) // TODO: Support Orthographic
  vec3 V = normalize(uCameraPos - vWorldPosition);
  vec3 N = normalize(worldNormal);

#ifdef ENV_MODE_REFLECTION
  vec3 reflectVec = reflect(-V, N);
#else
  vec3 reflectVec = refract(-V, N, uRefractionRatio);
#endif

  // TODO: Environment Map Rotation Uniform
  mat3 envMapRotation = mat3(1.0);
  reflectVec = envMapRotation * vec3(flipEnvMap * reflectVec.x, reflectVec.yz);

  vec4 env = texture(uEnvMap, reflectVec);
  return mix(vec4(1.0), env, uReflectivity);
}

)GLSL";

} // namespace blkhurst::shaders
