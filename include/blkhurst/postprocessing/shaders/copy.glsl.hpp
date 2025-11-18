#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string copy_frag = R"GLSL(

#include "io_fragment"
#include "uniforms_common"
#include "colorspace_fragment"
#include "tonemapping_fragment"

uniform sampler2D uInputBuffer;

void main() {
  vec4 texel = texture(uInputBuffer, vUv);

  // Tone Mapping + Color Space
  vec4 toneMapped = toneMapping(texel);
  FragColor = linearToOutput(toneMapped);
}

)GLSL";

} // namespace blkhurst::shaders
