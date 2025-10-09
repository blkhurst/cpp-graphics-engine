#pragma once

namespace blkhurst {

namespace uniforms {
// FrameUniforms (see uniform_blocks.hpp)
// constexpr const char* Time = "uTime";
// constexpr const char* Delta = "uDelta";
// constexpr const char* Mouse = "uMouse";
// constexpr const char* Resolution = "uResolution";
// constexpr const char* View = "uView";
// constexpr const char* Projection = "uProjection";
// constexpr const char* CameraPos = "uCameraPos";
// DrawUniforms
constexpr const char* Model = "uModel";
//
constexpr const char* Color = "uColor";
constexpr const char* Opacity = "uOpacity";
constexpr const char* AlphaTest = "uAlphaTest";
constexpr const char* NormalScale = "uNormalScale";
constexpr const char* Metalness = "uMetalness";
constexpr const char* Roughness = "uRoughness";
constexpr const char* AoIntensity = "uAoIntensity";
constexpr const char* EmissiveColor = "uEmissiveColor";
constexpr const char* EmissiveIntensity = "uEmissiveIntensity";
constexpr const char* UvTransform = "uUvTransform";
// Environment
constexpr const char* EnvRotation = "uEnvRotation";
constexpr const char* EnvIntensity = "uEnvIntensity";
constexpr const char* Reflectivity = "uReflectivity";       // BasicMaterial/Phong
constexpr const char* RefractionRatio = "uRefractionRatio"; // BasicMaterial/Phong
} // namespace uniforms

namespace slots {
constexpr int EnvMap = 0;
constexpr int BrdfLUT = 1;
constexpr int IrradianceMap = 2;
constexpr int PrefilterMap = 3;
constexpr int ShadowMap = 4;
constexpr int ColorMap = 5;
constexpr int AlphaMap = 6;
constexpr int NormalMap = 7;
constexpr int SpecularMap = 8;
constexpr int MetalnessMap = 9;
constexpr int RoughnessMap = 10;
constexpr int AoMap = 11;
constexpr int EmissiveMap = 12;
constexpr int DisplacementMap = 13;
// 16 guaranteed, often 32+
} // namespace slots

namespace samplers {
constexpr const char* EnvMap = "uEnvMap";
constexpr const char* BrdfLUT = "uBrdfLUT";
constexpr const char* IrradianceMap = "uIrradianceMap";
constexpr const char* PrefilterMap = "uPrefilterMap"; // Add LTC
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
constexpr const char* UseIBL = "USE_IBL";
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
constexpr const char* UseUvTransform = "USE_UV_TRANSFORM";
//
constexpr const char* EnvModeReflection = "ENV_MODE_REFLECTION";
constexpr const char* UseFlatShading = "FLAT_SHADING";
constexpr const char* UseVertexColor = "USE_VERTEX_COLOR";
constexpr const char* UseInstanceColor = "USE_INSTANCE_COLOR";
} // namespace defines

} // namespace blkhurst
