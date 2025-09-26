#pragma once
#include <glm/glm.hpp>

namespace blkhurst {

// UBOs use std140
// SSBOs use std430, but keep CPU 16-byte Struct Alignment Bytes for simplicity
constexpr int kCpuAlignment = 16;

enum class UniformBinding {
  Frame = 0,    // Per-frame UBO
  Draw = 1,     // Per-draw UBO (Unused but kept for prosperity)
  Lights = 2,   // Lights SSBO
  Instance = 3, // Instance SSBO
};

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
};

struct alignas(kCpuAlignment) DrawUniforms {
  glm::mat4 uModel;
  glm::mat4 uView;       // Temporary until FrameUniforms implements UBO
  glm::mat4 uProjection; // Temporary until FrameUniforms implements UBO
};

// Optionally, group in 16-byte chunks.
// Use `float pad0_` where needed to bring up to 16.
// Avoid `glm::mat3`

} // namespace blkhurst
