#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string skybox_vert = R"GLSL(

layout(location = 0) in vec3 aPosition;

out vec3 vPosition;

uniform mat4 uView;
uniform mat4 uProjection;

void main() {
  vPosition = aPosition;

  mat3 viewRotationOnly = mat3(uView);
  vec3 rotatedPosition = viewRotationOnly * aPosition;
  vec4 clipPosition = uProjection * vec4(rotatedPosition, 1.0);

  gl_Position = clipPosition.xyww;
}

)GLSL";

inline const std::string skybox_frag = R"GLSL(

out vec4 FragColor;

in vec3 vPosition;

uniform samplerCube uCubeMap;
uniform float uIntensity;
uniform mat3 uCubeMapRotation;
uniform float uFlipCubeMap;

void main() {
  vec3 direction = normalize(vPosition);
  vec3 sampleDirection = uCubeMapRotation * vec3(uFlipCubeMap * direction.x, direction.yz);
  vec4 sampleColor = texture(uCubeMap, sampleDirection);
  sampleColor.rgb *= uIntensity;

  FragColor = sampleColor;
}

)GLSL";

} // namespace blkhurst::shaders
