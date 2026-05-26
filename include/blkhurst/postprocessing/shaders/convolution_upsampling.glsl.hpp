#pragma once
#include <string>

/**
Convolution Upsampling - 9-Tap Tent Filter + Lerp
- https://www.froyok.fr/blog/2021-12-ue4-custom-bloom

This implementation uses the CoD-inspired 9-tap upsample filter with Froyok's internal blend.
  1) Upsample and convolve the previous (smaller) level with a 3x3 tent kernel
  2) Linearly blend the result with the current downsample level using uRadius.

Standard 3x3 Tent Filter:
[ 1 2 1 ] * ( 1 / 16 )
[ 2 4 2 ]
[ 1 2 1 ]

Use CLAMP_TO_EDGE on the sampler for the upsample stage (as Froyok suggests),
to avoid corner darkening.
*/

namespace blkhurst::shaders {

inline const std::string convolution_upsampling_vert = R"GLSL(

// #include "io_vertex"
layout(location = 0) in vec3 aPosition;

out vec2 vUv;
out vec2 vUv0;
out vec2 vUv1;
out vec2 vUv2;
out vec2 vUv3;
out vec2 vUv4;
out vec2 vUv5;
out vec2 vUv6;
out vec2 vUv7;

// uTexelSize = 1.0 / inputResolution
uniform vec2 uTexelSize;

void main() {
	vUv = aPosition.xy * 0.5 + 0.5;

  // 3x3 tent filter taps
	vUv0 = vUv + uTexelSize * vec2(-1.0, 1.0); // Top row
	vUv1 = vUv + uTexelSize * vec2(0.0, 1.0);
	vUv2 = vUv + uTexelSize * vec2(1.0, 1.0);
	vUv3 = vUv + uTexelSize * vec2(-1.0, 0.0); // Middle outer
	vUv4 = vUv + uTexelSize * vec2(1.0, 0.0);
	vUv5 = vUv + uTexelSize * vec2(-1.0, -1.0); // Bottom row
	vUv6 = vUv + uTexelSize * vec2(0.0, -1.0);
	vUv7 = vUv + uTexelSize * vec2(1.0, -1.0);

	gl_Position = vec4(aPosition.xy, 1.0, 1.0);
}

)GLSL";

inline const std::string convolution_upsampling_frag = R"GLSL(

layout(location = 0) out vec4 FragColor;

in vec2 vUv;
in vec2 vUv0;
in vec2 vUv1;
in vec2 vUv2;
in vec2 vUv3;
in vec2 vUv4;
in vec2 vUv5;
in vec2 vUv6;
in vec2 vUv7;

uniform sampler2D uInputBuffer; // Mip To Upsample
uniform sampler2D uSupportBuffer; // Current Level of Downsampled Mip to Blend With
uniform float uRadius; // Typical ~0.7-0.85

const float WEIGHT_CORNER = 0.0625;
const float WEIGHT_EDGE = 0.125;
const float WEIGHT_CENTER = 0.25;

vec4 weightedSample(const in vec2 uv, float weight) {
	return texture2D(uInputBuffer, uv) * weight;
}

void main() {
	vec4 c = vec4(0.0);

  // 3x3 tent filter
	c += weightedSample(vUv0, WEIGHT_CORNER);
	c += weightedSample(vUv1, WEIGHT_EDGE);
	c += weightedSample(vUv2, WEIGHT_CORNER);

	c += weightedSample(vUv3, WEIGHT_EDGE);
	c += weightedSample(vUv, WEIGHT_CENTER);
	c += weightedSample(vUv4, WEIGHT_EDGE);

	c += weightedSample(vUv5, WEIGHT_CORNER);
	c += weightedSample(vUv6, WEIGHT_EDGE);
	c += weightedSample(vUv7, WEIGHT_CORNER);

  // Blend Upsampled Result With Current Level Using uRadius
	vec4 baseColor = texture2D(uSupportBuffer, vUv);
	FragColor = mix(baseColor, c, uRadius);
}

)GLSL";

} // namespace blkhurst::shaders
