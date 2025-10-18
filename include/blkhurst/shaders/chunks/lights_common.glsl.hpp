#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string lights_common = R"GLSL(

// Directional lights (std430)
struct DirectionalLightGPU {
  vec3 color;
  float intensity;

  vec3 worldDirection;
  float pad0_;
};

// Point lights (std430)
struct PointLightGPU {
  vec3 color;
  float intensity;

  vec3 worldPosition;
  float decay;

  float distance; // 0 = infinite
  vec3 pad0_;
};

// UBO
layout(std140, binding = 2) uniform LightData {
  vec3 ambientColor;
  float ambientIntensity;

  int directionalCount;
  int pointCount;
  int pad0_;
  int pad1_;
} uLightData;

// SSBOs
layout(std430, binding = 3) buffer DirectionalLights {
  DirectionalLightGPU uDirectionalLights[];
} uDirectionalLights;

layout(std430, binding = 4) buffer PointLights {
  PointLightGPU uPointLights[];
} uPointLights;


float getDistanceAttenuation(float lightDistance, float cutoffDistance, float decayExponent) {
  float distanceFalloff = 1.0 / max(pow(lightDistance, decayExponent), 0.01);
  if (cutoffDistance > 0.0) {
    float x = clamp(1.0 - pow(lightDistance / cutoffDistance, 4.0), 0.0, 1.0);
    distanceFalloff *= pow(x, 2.0);
  }
  return distanceFalloff;
}

// ref: https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
float computeSpecularOcclusion( const in float dotNV, const in float ambientOcclusion, const in float roughness ) {
	return clamp( pow( dotNV + ambientOcclusion, exp2( - 16.0 * roughness - 1.0 ) ) - 1.0 + ambientOcclusion, 0.0, 1.0 );
}


)GLSL";

} // namespace blkhurst::shaders
