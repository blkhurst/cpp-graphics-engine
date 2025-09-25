#pragma once

namespace blkhurst {

namespace uniforms {
// FrameUniforms
// TODO: Move FrameUniforms/DrawUniforms into renderer/uniform_blocks.hpp
constexpr const char* Time = "uTime";
constexpr const char* Delta = "uDelta";
constexpr const char* Mouse = "uMouse";
constexpr const char* Resolution = "uResolution";
constexpr const char* View = "uView";
constexpr const char* Projection = "uProjection";
constexpr const char* CameraPos = "uCameraPos";
// DrawUniforms
constexpr const char* Model = "uModel";
//
constexpr const char* Color = "uColor";
constexpr const char* NormalScale = "uNormalScale";
// Environment
constexpr const char* Reflectivity = "uReflectivity";
constexpr const char* RefractionRatio = "uRefractionRatio";
} // namespace uniforms

namespace slots {
constexpr int EnvMap = 0;
constexpr int ShadowMap = 1;
constexpr int ColorMap = 2;
constexpr int AlphaMap = 3;
constexpr int NormalMap = 4;
constexpr int SpecularMap = 5;
constexpr int MetalnessMap = 6;
constexpr int RoughnessMap = 7;
constexpr int AoMap = 8;
constexpr int EmissiveMap = 9;
constexpr int DisplacementMap = 10;
} // namespace slots

namespace samplers {
constexpr const char* EnvMap = "uEnvMap"; // Add Irradiance/Prefiltered/BRDF/LTC
constexpr const char* ShadowMap = "uShadowMap";
constexpr const char* ColorMap = "uColorMap";
constexpr const char* AlphaMap = "uAlphaMap";
constexpr const char* NormalMap = "uNormalMap";
constexpr const char* SpecularMap = "uSpecularMap";
constexpr const char* MetalnessMap = "uMetalnessMap";
constexpr const char* RoughnessMap = "uRoughnessMap";
constexpr const char* AoMap = "uAoMap";
constexpr const char* EmissiveMap = "uEmissiveMap";
constexpr const char* DisplacementMap = "uDisplacementMap";
} // namespace samplers

namespace defines {
constexpr const char* UseEnvMap = "USE_ENVMAP";
constexpr const char* UseShadowMap = "USE_SHADOWMAP";
constexpr const char* UseColorMap = "USE_COLORMAP";
constexpr const char* UseAlphaMap = "USE_ALPHAMAP";
constexpr const char* UseNormalMap = "USE_NORMALMAP";
constexpr const char* UseSpecularMap = "USE_SPECULARMAP";
constexpr const char* UseMetalnessMap = "USE_METALNESSMAP";
constexpr const char* UseRoughnessMap = "USE_ROUGHNESSMAP";
constexpr const char* UseAoMap = "USE_AOMAP";
constexpr const char* UseEmissiveMap = "USE_EMISSIVEMAP";
constexpr const char* UseDisplacementMap = "USE_DISPLACEMENTMAP";
//
constexpr const char* EnvModeReflection = "ENV_MODE_REFLECTION";
constexpr const char* flatShading = "FLAT_SHADING";
} // namespace defines

} // namespace blkhurst
