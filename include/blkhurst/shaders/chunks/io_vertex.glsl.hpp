#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string io_vertex = R"GLSL(

// Attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aUv;
layout(location = 3) in vec3 aNormal;
layout(location = 4) in vec4 aInstanceColor;
layout(location = 5) in mat4 aInstanceMatrix; // Uses 5,6,7,8

// Out
out vec2 vUv;
out vec4 vColor;
out vec3 vWorldNormal;
out vec3 vWorldPosition;
out vec3 vViewPosition;
out vec4 vInstanceColor;

uniform mat3 uUvTransform;

// Extract to uv_vertex when supporting multiple maps
vec2 computeUv(vec2 uv) {
#ifdef USE_UV_TRANSFORM
  return (uUvTransform * vec3(uv, 1.0)).xy;
#else
  return uv;
#endif
}

void io_vertex(in mat4 model, in mat4 view, in mat4 projection) {
#ifdef USE_INSTANCING
  model = model * aInstanceMatrix;
#else
  model = model;
#endif

  // Positions
  vec4 worldPosition = model * vec4(aPosition, 1.0);
  vec4 viewPosition = view * worldPosition;

  // Normals
  mat3 worldNormalMatrix = mat3(transpose(inverse(model)));
  vec3 worldNormal = normalize(worldNormalMatrix * aNormal);

  // Out
  vUv = computeUv(aUv);
  vColor = aColor;
  vWorldPosition = worldPosition.xyz;
  vViewPosition = viewPosition.xyz;
  vWorldNormal = worldNormal;
  vInstanceColor = aInstanceColor;

  gl_Position = projection * viewPosition;
}

)GLSL";

} // namespace blkhurst::shaders
