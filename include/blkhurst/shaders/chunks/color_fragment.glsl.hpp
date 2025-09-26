#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string color_fragment = R"GLSL(

// *Depends on:
// io_fragment
//  in vec2 vUv;
//  in vec4 vColor;

uniform vec4 uColor;
uniform sampler2D uColorMap;
uniform sampler2D uAlphaMap;

vec4 computeColor() {
  vec4 color = uColor;

#ifdef USE_VERTEX_COLOR
  color *= vColor;
#endif

#ifdef USE_INSTANCE_COLOR
  color *= vInstanceColor;
#endif

#ifdef USE_COLORMAP
  vec4 texColor = texture(uColorMap, vUv);
  color *= texColor;
#endif

#ifdef USE_ALPHAMAP
  float alpha = texture(uAlphaMap, vUv).r;
  color.a *= alpha;
#endif

  return color;
}

)GLSL";

} // namespace blkhurst::shaders
