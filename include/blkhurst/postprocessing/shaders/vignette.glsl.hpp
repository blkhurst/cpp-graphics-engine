#pragma once
#include <string>

/**
Vignette pass.
- https://github.com/pmndrs/postprocessing/blob/main/src/effects/VignetteEffect.js

*/

namespace blkhurst::shaders {

inline const std::string vignette_frag = R"GLSL(

#include "io_fragment"

uniform sampler2D uInputBuffer;
uniform int uTechnique;
uniform float uOffset;
uniform float uDarkness;

const int kVignetteTechnique_Default = 0;
const int kVignetteTechnique_Eskil = 1;

float smoothstepRange(float edge0, float edge1, float value) {
  float t = clamp((value - edge0) / (edge1 - edge0), 0.0, 1.0);
  return t * t * (3.0 - 2.0 * t);
}

vec3 defaultVignette(vec3 color, vec2 uv) {
  float dist = distance(uv, vec2(0.5));
  return color * smoothstepRange(0.8, uOffset * 0.799, dist * (uDarkness + uOffset));
}

vec3 eskilVignette(vec3 color, vec2 uv) {
  vec2 coord = (uv - vec2(0.5)) * vec2(uOffset);
  return mix(color, vec3(1.0 - uDarkness), dot(coord, coord));
}

void main() {
  vec4 texel = texture(uInputBuffer, vUv);

  texel.rgb = uTechnique == kVignetteTechnique_Eskil
      ? eskilVignette(texel.rgb, vUv)
      : defaultVignette(texel.rgb, vUv);

  FragColor = texel;
}

)GLSL";

} // namespace blkhurst::shaders
