#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string common = R"GLSL(

#ifndef COMMON_GLSL
#define COMMON_GLSL

#define PI 3.141592653589793
#define PI2 6.283185307179586
#define PI_HALF 1.5707963267948966
#define RECIPROCAL_PI 0.3183098861837907
#define RECIPROCAL_PI2 0.15915494309189535
#define EPSILON 1e-6

// Match OpenGL cube convention (+X, -X, +Y, -Y, +Z, -Z)
vec3 faceToDirection(int face, vec2 uv) {
  vec2 centeredUv = uv * 2.0 - 1.0; // [0,1] -> [-1,1]
  if (face == 0)      return normalize(vec3( 1.0, -centeredUv.y, -centeredUv.x)); // +X
  else if (face == 1) return normalize(vec3(-1.0, -centeredUv.y, centeredUv.x));  // -X
  else if (face == 2) return normalize(vec3( centeredUv.x, 1.0, centeredUv.y));   // +Y
  else if (face == 3) return normalize(vec3( centeredUv.x, -1.0, -centeredUv.y)); // -Y
  else if (face == 4) return normalize(vec3( centeredUv.x, -centeredUv.y, 1.0));  // +Z
  else                return normalize(vec3(-centeredUv.x, -centeredUv.y, -1.0)); // -Z
}

#endif // COMMON_GLSL

)GLSL";

} // namespace blkhurst::shaders
