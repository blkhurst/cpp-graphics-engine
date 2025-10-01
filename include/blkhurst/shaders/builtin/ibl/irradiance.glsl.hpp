#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string irradiance_frag = R"GLSL(

#include "common"
#include "io_fragment"
#include "uniforms_common"

uniform int uFace;
uniform int uFaceSize;
uniform samplerCube uEnvMap;

void main() {
  // Mips are used to soften intense pin-point light sources and prevent bright dots/artifacts.
  // Use a constant mip level to prevent different levels per texel (which produces seams at the poles).
  float maxMip = floor(log2(float(uFaceSize))); // Same as Texture::calcMipLevels
  float mipLod = max(0.0, maxMip - 3.0); // Arbitrary, but -3 seems to prevent artifacts in most cases

  // Calculate all incoming radiance of the environment. The result of this radiance
  // is the radiance of light coming from -Normal direction, which is what
  // we use in the PBR shader to sample irradiance.
  vec3 N = faceToDirection(uFace, vUv);

  vec3 irradiance = vec3(0.0);

  // tangent space calculation from origin point
  vec3 up = vec3(0.0, 1.0, 0.0);
  vec3 right = normalize(cross(up, N));
  up = normalize(cross(N, right));

  float sampleDelta = 0.025;
  float nrSamples = 0.0;
  for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
    for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
      // spherical to cartesian (in tangent space)
      vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      // tangent space to world
      vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

      irradiance += textureLod(uEnvMap, sampleVec, mipLod).rgb * cos(theta) * sin(theta);
      nrSamples++;
    }
  }
  irradiance = PI * irradiance * (1.0 / float(nrSamples));

  FragColor = vec4(irradiance, 1.0);
}

)GLSL";

} // namespace blkhurst::shaders
