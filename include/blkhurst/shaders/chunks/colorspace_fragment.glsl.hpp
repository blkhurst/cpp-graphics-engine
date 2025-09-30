#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string colorspace_fragment = R"GLSL(

uniform int uOutputColorSpace;

// OutputColorSpace Enum
const int kOutputColorSpace_Linear = 0;
const int kOutputColorSpace_SRGB = 1;

vec4 sRGBToLinear(in vec4 srgb) {
  bvec3 cutoff = lessThanEqual(srgb.rgb, vec3(0.04045));
  vec3 low = srgb.rgb * 0.0773993808;
  vec3 high = pow(srgb.rgb * 0.9478672986 + vec3(0.0521327014), vec3(2.4));
  return vec4(mix(high, low, cutoff), srgb.a);
}

vec4 linearToSRGB(in vec4 linear) {
  bvec3 cutoff = lessThanEqual(linear.rgb, vec3(0.0031308));
  vec3 low = linear.rgb * 12.92;
  vec3 high = pow(linear.rgb, vec3(0.41666)) * 1.055 - vec3(0.055);
  return vec4(mix(high, low, cutoff), linear.a);
}

vec4 linearToOutput(vec4 linear) {
  if (uOutputColorSpace == kOutputColorSpace_Linear)
    return linear;
  else if (uOutputColorSpace == kOutputColorSpace_SRGB)
    return linearToSRGB(linear);

  return linear; // Fallback
}

)GLSL";

} // namespace blkhurst::shaders
