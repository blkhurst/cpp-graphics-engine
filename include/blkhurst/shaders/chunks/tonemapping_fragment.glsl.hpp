#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string tonemapping_fragment = R"GLSL(

uniform int uToneMappingMode;
uniform float uToneMappingExposure;

// ToneMapping Enum
const int kToneMappingMode_None = 0;
const int kToneMappingMode_Linear = 1;
const int kToneMappingMode_Neutral = 2;
const int kToneMappingMode_ACES = 3;

// Linear Tone Mapping
vec3 LinearToneMapping(vec3 color) {
  vec3 mapped = uToneMappingExposure * color;
  return clamp(mapped, 0.0, 1.0);
}

// Neutral Tone Mapping by Jim Hejl and Richard Burgess-Dawson
vec3 NeutralToneMapping(vec3 color) {
  const float StartCompression = 0.8 - 0.04;
  const float Desaturation = 0.15;
  color *= uToneMappingExposure;
  float x = min(color.r, min(color.g, color.b));
  float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
  color -= offset;
  float peak = max(color.r, max(color.g, color.b));
  if (peak < StartCompression)
    return color;
  float d = 1. - StartCompression;
  float newPeak = 1. - d * d / (peak + d - StartCompression);
  color *= newPeak / peak;
  float g = 1. - 1. / (Desaturation * (peak - newPeak) + 1.);
  return mix(color, vec3(newPeak), g);
}

// ACES Filmic Tone Mapping
// this implementation of ACES is modified to accommodate a brighter viewing environment.
// the scale factor of 1/0.6 is subjective.
vec3 RRTAndODTFit(vec3 v) {
  vec3 a = v * (v + 0.0245786) - 0.000090537;
  vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
  return a / b;
}
vec3 ACESFilmicToneMapping(vec3 color) {
  // sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
	const mat3 ACESInputMat = mat3(
		vec3( 0.59719, 0.07600, 0.02840 ), // transposed from source
		vec3( 0.35458, 0.90834, 0.13383 ),
		vec3( 0.04823, 0.01566, 0.83777 )
	);

	// ODT_SAT => XYZ => D60_2_D65 => sRGB
	const mat3 ACESOutputMat = mat3(
		vec3(  1.60475, -0.10208, -0.00327 ), // transposed from source
		vec3( -0.53108,  1.10813, -0.07276 ),
		vec3( -0.07367, -0.00605,  1.07602 )
	);

  color *= uToneMappingExposure / 0.6;
  color = ACESInputMat * color;

  // Apply RRT and ODT
  color = RRTAndODTFit(color);
  color = ACESOutputMat * color;

  // Clamp to [0, 1]
  return clamp(color, 0.0, 1.0);
}

vec3 toneMapping(vec3 linearRGB) {
  vec3 c = linearRGB;

  if (uToneMappingMode == kToneMappingMode_None)
    return c;
  else if (uToneMappingMode == kToneMappingMode_Linear)
    return LinearToneMapping(c);
  else if (uToneMappingMode == kToneMappingMode_Neutral)
    return NeutralToneMapping(c);
  else if (uToneMappingMode == kToneMappingMode_ACES)
    return ACESFilmicToneMapping(c);

  return c;
}

vec4 toneMapping(vec4 linearRGBA) {
  vec3 mapped = toneMapping(linearRGBA.rgb);
  return vec4(mapped, linearRGBA.a);
}


)GLSL";

} // namespace blkhurst::shaders
