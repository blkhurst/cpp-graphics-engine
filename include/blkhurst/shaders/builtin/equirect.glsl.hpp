#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string equirect_vert = R"GLSL(

#include "io_vertex"
#include "uniforms_common"

void main() {
  io_vertex(uModel, uView, uProjection); // Only used for attributes + vUv
  gl_Position = vec4(aPosition, 1.0); // Overwrite with clip position
}

)GLSL";

inline const std::string equirect_frag = R"GLSL(

#include "io_fragment"
#include "uniforms_common"

uniform int uFace; // [0..5] (+X, -X, +Y, -Y, +Z, -Z)
uniform sampler2D uEquirectT;

#define RECIPROCAL_PI 0.3183098861837907
#define RECIPROCAL_PI2 0.15915494309189535

// Equirectangular UV mapping; dir assumed to be normalized
vec2 equirectUv(in vec3 dir) {
  float u = atan(dir.x, dir.z) * RECIPROCAL_PI2 + 0.5; // Swapped X/Z to center +Z
  float v = asin(clamp(dir.y, -1.0, 1.0)) * RECIPROCAL_PI + 0.5;
  return vec2(u, v);
}

vec3 faceToDirection(int face, vec2 uv) {
  vec2 centeredUv = uv * 2.0 - 1.0; // [0,1] -> [-1,1]
  // Match OpenGL cube convention
  if (face == 0)      return normalize(vec3( 1.0, -centeredUv.y, -centeredUv.x)); // +X
  else if (face == 1) return normalize(vec3(-1.0, -centeredUv.y, centeredUv.x));  // -X
  else if (face == 2) return normalize(vec3( centeredUv.x, 1.0, centeredUv.y));   // +Y
  else if (face == 3) return normalize(vec3( centeredUv.x, -1.0, -centeredUv.y)); // -Y
  else if (face == 4) return normalize(vec3( centeredUv.x, -centeredUv.y, 1.0));  // +Z
  else                return normalize(vec3(-centeredUv.x, -centeredUv.y, -1.0)); // -Z
}

void main() {
  vec3 direction = faceToDirection(uFace, vUv);
  vec2 sampleUV = equirectUv(direction);

  // Ensure we convert using LOD 0 for full resolution
  FragColor = textureLod(uEquirectT, sampleUV, 0.0);
}

)GLSL";

} // namespace blkhurst::shaders
