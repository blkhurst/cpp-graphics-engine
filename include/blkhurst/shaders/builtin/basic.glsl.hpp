#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string basic_vert = R"GLSL(

#include "io_vertex"
#include "uniforms_common"

void main() {
  io_vertex(uModel, uView, uProjection);
}

)GLSL";

inline const std::string basic_frag = R"GLSL(

#include "io_fragment"
#include "uniforms_common"
#include "normal_fragment"
#include "color_fragment"
#include "envmap_fragment"
#include "tonemapping_fragment"
#include "colorspace_fragment"

void main() {
  vec3 worldNormal = computeWorldNormal();

  vec3 base = computeColor();
  float alpha = computeAlpha();
  vec4 env = computeEnv(worldNormal);

  vec4 color = vec4(base * env.rgb, alpha);

  vec4 toneMapped = toneMapping(color);
  FragColor = linearToOutput(toneMapped);
}

)GLSL";

} // namespace blkhurst::shaders
