#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string pbr_vert = R"GLSL(

#include "io_vertex"
#include "uniforms_common"

void main() {
  io_vertex(uModel, uView, uProjection);
}

)GLSL";

inline const std::string pbr_frag = R"GLSL(

#include "common"
#include "io_fragment"
#include "uniforms_common"
#include "normal_fragment"
#include "color_fragment"
#include "envmap_fragment"
#include "tonemapping_fragment"
#include "colorspace_fragment"

#include "lights_common"
#include "pbr_common"
#include "pbr_fragment"
#include "ibl_fragment"

void main() {
  vec3 albedo = computeColor();
  float alpha = computeAlpha();
  float metalness = computeMetalness();
  float roughness = computeRoughness();
  float ao = computeAo();
  vec3 emission = computeEmissive();

  vec3 N = computeWorldNormal();
  vec3 V = normalize(uCameraPos - vWorldPosition);
  vec3 R = reflect(-V, N);

  vec3 F0 = computeF0(albedo, metalness);

  vec3 Lo = accumulateLights(N, V, R, F0, albedo, metalness, roughness, ao);

  vec3 ibl = computeIBL(N, V, R, F0, albedo, metalness, roughness, ao);
  vec3 ambient = ibl;

  vec4 color = vec4(ambient + Lo + emission, alpha);

  // Tone Mapping + Color Space
  vec4 toneMapped = toneMapping(color);
  FragColor = linearToOutput(toneMapped);
}

)GLSL";

} // namespace blkhurst::shaders
