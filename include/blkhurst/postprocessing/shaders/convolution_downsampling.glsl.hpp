#pragma once
#include <string>

/**
Convolution Downsampling - 13-Tap Kernel
- https://www.froyok.fr/blog/2021-12-ue4-custom-bloom

This implementation uses the CoD-inspired 13-tap sample pattern with Froyok's adjusted weights.

Froyok's kernel:
  - 4 inner diagonal taps contributing 50% (1/4*0.5 = 0.125 each)
  - 9 outer 3x3 grid taps contributing 50% (1/9*0.5 = 0.0555555 each)

Each tap emulates CLAMP_TO_BORDER (black) to avoid bright wrap-in at edges,
which Froyok recommends for downsampling.

We avoid auto-generated mipmaps since they are typically box filters. The 13-tap kernel
prevents temporal shimmering and pulsating artifacts during movement.
*/

namespace blkhurst::shaders {

inline const std::string convolution_downsampling_vert = R"GLSL(

// #include "io_vertex"
layout(location = 0) in vec3 aPosition;

out vec2 vUv;
out vec2 vUv00;
out vec2 vUv01;
out vec2 vUv02;
out vec2 vUv03;
out vec2 vUv04;
out vec2 vUv05;
out vec2 vUv06;
out vec2 vUv07;
out vec2 vUv08;
out vec2 vUv09;
out vec2 vUv10;
out vec2 vUv11;

// uTexelSize = 1.0 / inputResolution
uniform vec2 uTexelSize;

void main() {
	vUv = aPosition.xy * 0.5 + 0.5;

  // Inner 4 diagonal taps
	vUv00 = vUv + uTexelSize * vec2(-1.0, 1.0);
	vUv01 = vUv + uTexelSize * vec2(1.0, 1.0);
	vUv02 = vUv + uTexelSize * vec2(-1.0, -1.0);
	vUv03 = vUv + uTexelSize * vec2(1.0, -1.0);

  // Outer 3x3 taps
	vUv04 = vUv + uTexelSize * vec2(-2.0, 2.0); // Top row
	vUv05 = vUv + uTexelSize * vec2(0.0, 2.0);
	vUv06 = vUv + uTexelSize * vec2(2.0, 2.0);
	vUv07 = vUv + uTexelSize * vec2(-2.0, 0.0); // Middle outer
	vUv08 = vUv + uTexelSize * vec2(2.0, 0.0);
	vUv09 = vUv + uTexelSize * vec2(-2.0, -2.0); // Bottom row
	vUv10 = vUv + uTexelSize * vec2(0.0, -2.0);
	vUv11 = vUv + uTexelSize * vec2(2.0, -2.0);

	gl_Position = vec4(aPosition.xy, 1.0, 1.0);
}

)GLSL";

inline const std::string convolution_downsampling_frag = R"GLSL(

layout(location = 0) out vec4 FragColor;

in vec2 vUv;
in vec2 vUv00;
in vec2 vUv01;
in vec2 vUv02;
in vec2 vUv03;
in vec2 vUv04;
in vec2 vUv05;
in vec2 vUv06;
in vec2 vUv07;
in vec2 vUv08;
in vec2 vUv09;
in vec2 vUv10;
in vec2 vUv11;

uniform sampler2D uInputBuffer;

const float WEIGHT_INNER = 0.125;     // (1 / 4) * 0.5 = 0.125
const float WEIGHT_OUTER = 0.0555555; // (1 / 9) * 0.5 = 0.0555555

float clampToBorder(const in vec2 uv) {
	return float(uv.s >= 0.0 && uv.s <= 1.0 && uv.t >= 0.0 && uv.t <= 1.0);
}

vec4 weightedSample(const in vec2 uv, float weight) {
	return texture2D(uInputBuffer, uv) * weight * clampToBorder(uv);
}

void main() {
	vec4 c = vec4(0.0);

  // Inner 4 Diagonal Taps
	c += weightedSample(vUv00, WEIGHT_INNER);
	c += weightedSample(vUv01, WEIGHT_INNER);
	c += weightedSample(vUv02, WEIGHT_INNER);
	c += weightedSample(vUv03, WEIGHT_INNER);

  // Outer 3x3 Taps, including center
	c += weightedSample(vUv04, WEIGHT_OUTER);
	c += weightedSample(vUv05, WEIGHT_OUTER);
	c += weightedSample(vUv06, WEIGHT_OUTER);

	c += weightedSample(vUv07, WEIGHT_OUTER);
	c += weightedSample(vUv, WEIGHT_OUTER);
	c += weightedSample(vUv08, WEIGHT_OUTER);

	c += weightedSample(vUv09, WEIGHT_OUTER);
	c += weightedSample(vUv10, WEIGHT_OUTER);
	c += weightedSample(vUv11, WEIGHT_OUTER);

	FragColor = c;
}

)GLSL";

} // namespace blkhurst::shaders

/**

CoD 13-Tap Method For Reference

const float WEIGHT_OUTER = 0.03125; // (1 / 16) * 0.5 = 0.03125

// Top-left group
c += weightedSample(vUv04, WEIGHT_OUTER);
c += weightedSample(vUv05, WEIGHT_OUTER);
c += weightedSample(vUv07, WEIGHT_OUTER);
c += weightedSample(vUv, WEIGHT_OUTER);

// Top-right group
c += weightedSample(vUv05, WEIGHT_OUTER);
c += weightedSample(vUv06, WEIGHT_OUTER);
c += weightedSample(vUv, WEIGHT_OUTER);
c += weightedSample(vUv08, WEIGHT_OUTER);

// Bottom-right group
c += weightedSample(vUv, WEIGHT_OUTER);
c += weightedSample(vUv08, WEIGHT_OUTER);
c += weightedSample(vUv10, WEIGHT_OUTER);
c += weightedSample(vUv11, WEIGHT_OUTER);

// Bottom-left group
c += weightedSample(vUv07, WEIGHT_OUTER);
c += weightedSample(vUv, WEIGHT_OUTER);
c += weightedSample(vUv09, WEIGHT_OUTER);
c += weightedSample(vUv10, WEIGHT_OUTER);

*/
