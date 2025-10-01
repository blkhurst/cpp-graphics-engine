#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string equirect_frag = R"GLSL(

#include "io_fragment"
#include "uniforms_common"
#include "common"

uniform int uFace; // [0..5] (+X, -X, +Y, -Y, +Z, -Z)
uniform sampler2D uEquirectT;

// Equirectangular UV mapping; dir assumed to be normalized
vec2 equirectUv(in vec3 dir) {
  float u = atan(dir.x, dir.z) * RECIPROCAL_PI2 + 0.5; // Swapped X/Z to center +Z
  float v = asin(clamp(dir.y, -1.0, 1.0)) * RECIPROCAL_PI + 0.5;
  return vec2(u, v);
}

void main() {
  vec3 direction = faceToDirection(uFace, vUv);
  vec2 sampleUV = equirectUv(direction);

  // Ensure we convert using LOD 0 for full resolution
  FragColor = textureLod(uEquirectT, sampleUV, 0.0);
}

)GLSL";

} // namespace blkhurst::shaders
