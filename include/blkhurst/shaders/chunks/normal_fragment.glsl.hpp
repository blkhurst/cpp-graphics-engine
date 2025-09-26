#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string normal_fragment = R"GLSL(

// *Depends on:
//  io_fragment
//    in vec2 vUv;
//    in vec3 vWorldNormal;
//    in vec3 vWorldPosition;
//  uniforms_common
//    uniform mat4 uView;
//  defines
//    FLAT_SHADING
//    USE_NORMALMAP

uniform sampler2D uNormalMap;
uniform float uNormalScale;

void computeGeometryNormal(out vec3 worldNormal) {
#ifdef FLAT_SHADING

  vec3 fdx = dFdx(vWorldPosition);
  vec3 fdy = dFdy(vWorldPosition);
  worldNormal = normalize(cross(fdx, fdy));

#else

  worldNormal = normalize(vWorldNormal);

#endif

  // Invert worldNormal if backface
  float faceDirection = gl_FrontFacing ? 1.0 : -1.0;
  worldNormal *= faceDirection;
}

// Compute Tangent-Bitangent-Normal matrix - World Space
void computeTBN(in vec3 worldNormal, out mat3 tbn) {
  vec3 q0 = dFdx(vWorldPosition);
  vec3 q1 = dFdy(vWorldPosition);
  vec2 st0 = dFdx(vUv);
  vec2 st1 = dFdy(vUv);

  vec3 N = worldNormal; // normalized

  vec3 q1perp = cross(q1, N);
  vec3 q0perp = cross(N, q0);

  vec3 T = q1perp * st0.x + q0perp * st1.x;
  vec3 B = q1perp * st0.y + q0perp * st1.y;

  float det = max(dot(T, T), dot(B, B));
  float scale = (det == 0.0) ? 0.0 : inversesqrt(det);

  tbn = mat3(T * scale, B * scale, N);
}

void computeNormal(inout vec3 worldNormal, out vec3 viewNormal, in mat3 tbn) {
#ifdef USE_NORMALMAP

  // Sample uNormalMap & apply uNormalScale
  vec3 mapN = texture(uNormalMap, vUv).xyz * 2.0 - 1.0;
  mapN.xy *= uNormalScale;

  // Tangent to World
  worldNormal = normalize(tbn * mapN);

#endif

  // View Normal
  viewNormal = normalize(mat3(uView) * worldNormal);
}

)GLSL";

} // namespace blkhurst::shaders
