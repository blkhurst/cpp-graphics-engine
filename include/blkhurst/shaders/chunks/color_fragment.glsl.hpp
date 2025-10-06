#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string color_fragment = R"GLSL(

// *Depends on:
// io_fragment
//  in vec2 vUv;
//  in vec3 vColor;

uniform vec3 uColor;
uniform sampler2D uColorMap;
uniform sampler2D uAlphaMap;

vec3 computeColor() {
  vec3 color = uColor;

#ifdef USE_VERTEX_COLOR
  color *= vColor;
#endif

#ifdef USE_INSTANCE_COLOR
  color *= vInstanceColor;
#endif

#ifdef USE_COLORMAP
  vec3 colorMap = texture(uColorMap, vUv).rgb;
  color *= colorMap;
#endif

  return color;
}

// *Depends on:
//  io_fragment
//    in vec2 vUv;
//  color_fragment
//    uniform sampler2D uColorMap;

uniform float uOpacity;
uniform float uAlphaTest;

float computeAlpha() {
  float alpha = uOpacity;

#ifdef USE_COLORMAP
  float colorMap = texture(uColorMap, vUv).a;
  alpha *= colorMap;
#endif

#ifdef USE_ALPHAMAP
  float alphaMap = texture(uAlphaMap, vUv).r;
  alpha *= alphaMap;
#endif

  if (uAlphaTest >= 0.0 && alpha < uAlphaTest) {
    discard;
  }

  return alpha;
}

)GLSL";

} // namespace blkhurst::shaders
