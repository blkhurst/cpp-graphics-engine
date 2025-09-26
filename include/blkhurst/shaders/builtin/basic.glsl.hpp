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

void main() {
  mat3 tbn;
  vec3 worldNormal;
  vec3 viewNormal;
  computeGeometryNormal(worldNormal);
  computeTBN(worldNormal, tbn);
  computeNormal(worldNormal, viewNormal, tbn);

  vec4 base = computeColor();
  vec4 env = computeEnv(worldNormal);

  vec3 rgb = base.rgb * env.rgb;
  FragColor = vec4(rgb, base.a);
}

)GLSL";

} // namespace blkhurst::shaders
