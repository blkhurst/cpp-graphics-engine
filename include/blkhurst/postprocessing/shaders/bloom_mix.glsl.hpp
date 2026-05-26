#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string bloom_mix_frag = R"GLSL(

#include "io_fragment"

uniform sampler2D uInputBuffer;
uniform sampler2D uBloomBuffer;
uniform float uBloomIntensity;

void main() {
  vec4 texel = texture(uInputBuffer, vUv);
  vec4 bloom = texture(uBloomBuffer, vUv);
  FragColor = mix(texel, bloom, uBloomIntensity);
}

)GLSL";

} // namespace blkhurst::shaders
