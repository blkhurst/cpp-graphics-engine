
#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string loading_screen_frag = R"GLSL(

out vec4 fragColor;
vec2 fragCoord = gl_FragCoord.xy;

#include "uniforms_common"
uniform float uOpacity;

const vec2 POSITION = vec2(81, 80);
const float SIZE = 40.0;
const float INNER_SIZE = 8.0;
const vec3 LIGHT_COLOR = vec3(0.8);
const vec3 DARK_COLOR = vec3(0.15);
const vec3 BG_COLOR = vec3(0.1);

void main() {
  float timeFactor = uTime * 7.0;
  float len = length(fragCoord - vec2(uResolution.x - POSITION.x, POSITION.y));

  if (len < SIZE) {
    float angle = dot(vec2(sin(timeFactor), cos(timeFactor)),
                      normalize(fragCoord - vec2(uResolution.x - POSITION.x, POSITION.y)));
    if (angle > 0.85)
      fragColor = vec4(LIGHT_COLOR, 1.0);
    else
      fragColor = vec4(DARK_COLOR, 1.0);
  } else {
    fragColor = vec4(BG_COLOR, 1.0);
  }

  len = length(fragCoord - vec2(uResolution.x - POSITION.x, POSITION.y));
  if (len < (SIZE - INNER_SIZE)) {
    fragColor = vec4(BG_COLOR, 1.0);
  }

  fragColor *= min(timeFactor, 1.0);
  fragColor.a = uOpacity;
}

)GLSL";

} // namespace blkhurst::shaders
