#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string uniforms_common = R"GLSL(

layout(std140, binding = 0) uniform FrameBlock {
  float uTime;
  float uDelta;
  vec2  uMouse;

  vec2  uResolution;
  float pad0_;
  float pad1_;

  mat4  uView;
  mat4  uProjection;

  vec3  uCameraPos;
  int   uIsOrthographic;

  float uToneMappingExposure;
  int   uToneMappingMode;
  int   uOutputColorSpace;
  float pad2_;
} uFrame;


// FrameUniforms
float uTime = uFrame.uTime;
float uDelta = uFrame.uDelta;
vec2 uMouse = uFrame.uMouse;
vec2 uResolution = uFrame.uResolution;
mat4 uView = uFrame.uView;
mat4 uProjection = uFrame.uProjection;
vec3 uCameraPos = uFrame.uCameraPos;
bool uIsOrthographic = uFrame.uIsOrthographic == 1;
float uToneMappingExposure = uFrame.uToneMappingExposure;
int uToneMappingMode = uFrame.uToneMappingMode;
int uOutputColorSpace = uFrame.uOutputColorSpace;

// DrawUniforms
uniform mat4 uModel;

)GLSL";

} // namespace blkhurst::shaders
