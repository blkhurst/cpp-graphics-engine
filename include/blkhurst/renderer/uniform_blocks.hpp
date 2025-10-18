#pragma once
#include <glm/glm.hpp>

namespace blkhurst {

// UBOs use std140
// SSBOs use std430, but keep CPU 16-byte Struct Alignment Bytes for simplicity
constexpr int kCpuAlignment = 16;

namespace uniform_bindings {
constexpr static int Frame = 0;             // UBO
constexpr static int Draw = 1;              // UBO (optional since per-draw)
constexpr static int LightData = 2;         // UBO
constexpr static int DirectionalLights = 3; // SSBO
constexpr static int PointLights = 4;       // SSBO
constexpr static int Instance = 5;          // SSBO
}; // namespace uniform_bindings

struct alignas(kCpuAlignment) FrameUniforms {
  float uTime;      // 4
  float uDelta;     // 4
  glm::vec2 uMouse; // 8

  glm::vec2 uResolution; // 8
  float pad0_;           // 4
  float pad1_;           // 4

  glm::mat4 uView;       // 16
  glm::mat4 uProjection; // 16

  glm::vec3 uCameraPos; // 12
  int uIsOrthographic;  // 4

  // Renderer Settings
  float uToneMappingExposure; // 4
  int uToneMappingMode;       // 4
  int uOutputColorSpace;      // 4
  float pad2_;                // 4
};

struct alignas(kCpuAlignment) DrawUniforms {
  glm::mat4 uModel;
};

struct alignas(kCpuAlignment) LightDataGPU {
  glm::vec3 ambientColor; // 12
  float ambientIntensity; // 4

  int directionalCount; // 4
  int pointCount;       // 4
  int pad0_;            // 4
  int pad1_;            // 4
};

struct alignas(kCpuAlignment) DirectionalLightGPU {
  glm::vec3 color; // 12
  float intensity; // 4

  glm::vec3 worldDirection; // 12 (Light to Target)
  float pad0_;              // 4
};

struct alignas(kCpuAlignment) PointLightGPU {
  glm::vec3 color; // 12
  float intensity; // 4

  glm::vec3 worldPosition; // 12
  float decay;             // 4

  float distance;  // 4
  glm::vec3 pad0_; // 12
};

// Optionally, group in 16-byte chunks.
// Use `float pad0_` where needed to bring up to 16.
// Avoid `glm::mat3`

} // namespace blkhurst
