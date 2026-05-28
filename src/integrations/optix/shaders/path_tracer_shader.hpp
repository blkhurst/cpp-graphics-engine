#pragma once

#include <string>

namespace blkhurst::optix::shaders {

inline const char* types_and_math = R"CUDA_CHUNK(#include <cuda_fp16.h>
#include <optix.h>
#include <optix_device.h>
#include <texture_types.h>

struct half4 {
  half x;
  half y;
  half z;
  half w;
};

static __forceinline__ __device__ half4 make_half4(half x, half y, half z, half w) {
  half4 value;
  value.x = x;
  value.y = y;
  value.z = z;
  value.w = w;
  return value;
}

struct MaterialData {
  float3 albedo;
  float3 emission;
  float metallic;
  float roughness;
  float opacity;
  float alphaTest;
  float normalScale;
  cudaTextureObject_t albedoMap;
  cudaTextureObject_t alphaMap;
  cudaTextureObject_t normalMap;
  cudaTextureObject_t metalnessMap;
  cudaTextureObject_t roughnessMap;
  cudaTextureObject_t aoMap;
  cudaTextureObject_t emissiveMap;
  unsigned int hasAlbedoMap;
  unsigned int hasAlphaMap;
  unsigned int hasNormalMap;
  unsigned int hasMetalnessMap;
  unsigned int hasRoughnessMap;
  unsigned int hasAoMap;
  unsigned int hasEmissiveMap;
};

struct TriangleLight {
  float3 v0;
  float3 v1;
  float3 v2;
  float3 normal;
  float area;
  unsigned int materialIndex;
};

struct Params {
  half4* output;
  float4* accumulation;
  unsigned int width;
  unsigned int height;
  unsigned int samplesPerPixel;
  unsigned int maxBounces;
  unsigned int frameIndex;
  unsigned int accumulate;
  unsigned int integratorMode;
  unsigned int samplingMode;
  unsigned int environmentMode;
  unsigned int materialMode;
  unsigned int misMode;
  unsigned int debugView;
  unsigned int enableTextures;
  unsigned int enableNormalMaps;
  unsigned int enableAlpha;
  unsigned int enableMirrorReflection;
  unsigned int enableDirectLighting;
  unsigned int enableShadowRays;
  unsigned int enableEmissiveLights;
  unsigned int enableRussianRoulette;
  OptixTraversableHandle traversable;
  float3 cameraPosition;
  float4 invViewProjection[4];
  float3* vertices;
  float2* uvs;
  float3* normals;
  uint3* indices;
  unsigned int* materialIndices;
  MaterialData* materials;
  TriangleLight* lights;
  unsigned int lightCount;
  cudaTextureObject_t environmentMap;
  unsigned int hasEnvironmentMap;
  float3 environmentColor;
  float environmentIntensity;
};

extern "C" {
__constant__ Params params;
}

constexpr unsigned int INTEGRATOR_FIRST_HIT = 0u;
constexpr unsigned int INTEGRATOR_PATH_TRACING = 1u;
constexpr unsigned int INTEGRATOR_DIRECT_LIGHTING = 2u;
constexpr unsigned int INTEGRATOR_MIS = 3u;

constexpr unsigned int SAMPLING_UNIFORM = 0u;
constexpr unsigned int SAMPLING_COSINE = 1u;
constexpr unsigned int SAMPLING_GGX = 2u;
constexpr unsigned int SAMPLING_GGX_VNDF = 3u;

constexpr unsigned int ENVIRONMENT_OFF = 0u;
constexpr unsigned int ENVIRONMENT_FLAT = 1u;
constexpr unsigned int ENVIRONMENT_HDRI = 2u;

constexpr unsigned int MATERIAL_LAMBERT = 0u;
constexpr unsigned int MATERIAL_PBR_GGX = 1u;

constexpr unsigned int MIS_OFF = 0u;
constexpr unsigned int MIS_BALANCE = 1u;
constexpr unsigned int MIS_POWER = 2u;

constexpr unsigned int DEBUG_BEAUTY = 0u;
constexpr unsigned int DEBUG_NORMALS = 1u;
constexpr unsigned int DEBUG_ALBEDO = 2u;
constexpr unsigned int DEBUG_DEPTH = 3u;
constexpr unsigned int DEBUG_DIRECT = 4u;
constexpr unsigned int DEBUG_INDIRECT = 5u;
constexpr unsigned int DEBUG_PDF = 6u;
constexpr unsigned int DEBUG_MIS_WEIGHT = 7u;
constexpr unsigned int DEBUG_BOUNCE_COUNT = 8u;

static __forceinline__ __device__ float uintToFloat(unsigned int value) {
  return __uint_as_float(value);
}

static __forceinline__ __device__ unsigned int floatToUint(float value) {
  return __float_as_uint(value);
}

static __forceinline__ __device__ unsigned int pcgHash(unsigned int value) {
  unsigned int state = value * 747796405u + 2891336453u;
  unsigned int word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  return (word >> 22u) ^ word;
}

static __forceinline__ __device__ unsigned int initRng(unsigned int pixelIndex,
                                                       unsigned int sampleIndex,
                                                       unsigned int frameIndex) {
  unsigned int seed = pixelIndex;
  seed ^= pcgHash(sampleIndex + 0x9e3779b9u);
  seed ^= pcgHash(frameIndex + 0x85ebca6bu);
  return pcgHash(seed);
}

static __forceinline__ __device__ float rand(unsigned int& state) {
  state = state * 747796405u + 2891336453u;
  unsigned int word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
  word = (word >> 22u) ^ word;
  return float(word) * 2.3283064365386963e-10f;
}

static __forceinline__ __device__ float3 cross3(float3 a, float3 b) {
  return make_float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

static __forceinline__ __device__ float dot3(float3 a, float3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

static __forceinline__ __device__ float3 normalize3(float3 value) {
  float invLen = rsqrtf(fmaxf(dot3(value, value), 1e-20f));
  return make_float3(value.x * invLen, value.y * invLen, value.z * invLen);
}

static __forceinline__ __device__ float3 add3(float3 a, float3 b) {
  return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static __forceinline__ __device__ float3 sub3(float3 a, float3 b) {
  return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static __forceinline__ __device__ float3 mul3(float3 value, float scale) {
  return make_float3(value.x * scale, value.y * scale, value.z * scale);
}

static __forceinline__ __device__ float3 mul3(float3 a, float3 b) {
  return make_float3(a.x * b.x, a.y * b.y, a.z * b.z);
}

static __forceinline__ __device__ float3 div3(float3 value, float scale) {
  return make_float3(value.x / scale, value.y / scale, value.z / scale);
}

static __forceinline__ __device__ float3 max3(float3 value, float minValue) {
  return make_float3(fmaxf(value.x, minValue), fmaxf(value.y, minValue), fmaxf(value.z, minValue));
}

static __forceinline__ __device__ float luminance(float3 value) {
  return value.x * 0.2126f + value.y * 0.7152f + value.z * 0.0722f;
}

static __forceinline__ __device__ float3 lerp3(float3 a, float3 b, float t) {
  return add3(mul3(a, 1.0f - t), mul3(b, t));
}

static __forceinline__ __device__ float3 reflect3(float3 incident, float3 normal) {
  return sub3(incident, mul3(normal, 2.0f * dot3(incident, normal)));
}

static __forceinline__ __device__ float3 srgbToLinear(float3 value) {
  return make_float3(powf(value.x, 2.2f), powf(value.y, 2.2f), powf(value.z, 2.2f));
}

static __forceinline__ __device__ float4 sampleTexture(cudaTextureObject_t texture, float2 uv) {
  return tex2D<float4>(texture, uv.x, uv.y);
}

)CUDA_CHUNK";

inline const char* material_sampling = R"CUDA_CHUNK(struct MaterialSample {
  float3 albedo;
  float3 emission;
  float metallic;
  float roughness;
  float opacity;
  float ao;
  float3 tangentNormal;
};

static __forceinline__ __device__ MaterialSample sampleMaterial(MaterialData material, float2 uv) {
  MaterialSample sample;
  sample.albedo = material.albedo;
  sample.emission = material.emission;
  sample.metallic = material.metallic;
  sample.roughness = material.roughness;
  sample.opacity = material.opacity;
  sample.ao = 1.0f;
  sample.tangentNormal = make_float3(0.0f, 0.0f, 1.0f);

  if (params.enableTextures != 0u && material.hasAlbedoMap != 0u) {
    float4 texel = sampleTexture(material.albedoMap, uv);
    sample.albedo = mul3(sample.albedo, srgbToLinear(make_float3(texel.x, texel.y, texel.z)));
    sample.opacity *= texel.w;
  }
  if (params.enableAlpha != 0u && material.hasAlphaMap != 0u) {
    sample.opacity *= sampleTexture(material.alphaMap, uv).x;
  }
  if (params.enableTextures != 0u && material.hasMetalnessMap != 0u) {
    sample.metallic *= sampleTexture(material.metalnessMap, uv).z;
  }
  if (params.enableTextures != 0u && material.hasRoughnessMap != 0u) {
    sample.roughness *= sampleTexture(material.roughnessMap, uv).y;
  }
  if (params.enableTextures != 0u && material.hasAoMap != 0u) {
    sample.ao = sampleTexture(material.aoMap, uv).x;
  }
  if (params.enableTextures != 0u && material.hasEmissiveMap != 0u) {
    float4 texel = sampleTexture(material.emissiveMap, uv);
    sample.emission = mul3(sample.emission, srgbToLinear(make_float3(texel.x, texel.y, texel.z)));
  }
  if (params.enableNormalMaps != 0u && material.hasNormalMap != 0u) {
    float4 texel = sampleTexture(material.normalMap, uv);
    sample.tangentNormal = normalize3(make_float3((texel.x * 2.0f - 1.0f) * material.normalScale,
                                                  (texel.y * 2.0f - 1.0f) * material.normalScale,
                                                  texel.z * 2.0f - 1.0f));
  }

  sample.albedo = mul3(sample.albedo, sample.ao);
  sample.roughness = fminf(fmaxf(sample.roughness, 0.0f), 1.0f);
  sample.metallic = fminf(fmaxf(sample.metallic, 0.0f), 1.0f);
  sample.opacity = fminf(fmaxf(sample.opacity, 0.0f), 1.0f);
  return sample;
}

static __forceinline__ __device__ unsigned int hashRay(float3 origin, float3 direction,
                                                       unsigned int primitiveIndex) {
  unsigned int seed = primitiveIndex * 9781u + 17u;
  seed ^= floatToUint(origin.x) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
  seed ^= floatToUint(origin.y) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
  seed ^= floatToUint(origin.z) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
  seed ^= floatToUint(direction.x) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
  seed ^= floatToUint(direction.y) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
  seed ^= floatToUint(direction.z) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u);
  return seed;
}

)CUDA_CHUNK";

inline const char* environment =
    R"CUDA_CHUNK(static __forceinline__ __device__ float3 sampleEnvironment(float3 direction) {
  if (params.environmentMode == ENVIRONMENT_OFF) {
    return make_float3(0.0f, 0.0f, 0.0f);
  }

  if (params.environmentMode == ENVIRONMENT_HDRI && params.hasEnvironmentMap != 0u) {
    constexpr float invPi = 0.31830988618f;
    constexpr float invTwoPi = 0.15915494309f;
    float u = atan2f(-direction.x, direction.z) * invTwoPi + 0.5f;
    float v = asinf(fminf(fmaxf(direction.y, -1.0f), 1.0f)) * invPi + 0.5f;
    float4 texel = sampleTexture(params.environmentMap, make_float2(u, v));
    return mul3(make_float3(texel.x, texel.y, texel.z), params.environmentIntensity);
  }

  float horizon = 0.5f + 0.5f * direction.y;
  float3 ground = mul3(params.environmentColor, 0.35f);
  float3 sky = params.environmentColor;
  return mul3(lerp3(ground, sky, horizon), params.environmentIntensity);
}

static __forceinline__ __device__ float4 mulMat(float4 rows[4], float4 value) {
  return make_float4(
    rows[0].x * value.x + rows[0].y * value.y + rows[0].z * value.z + rows[0].w * value.w,
    rows[1].x * value.x + rows[1].y * value.y + rows[1].z * value.z + rows[1].w * value.w,
    rows[2].x * value.x + rows[2].y * value.y + rows[2].z * value.z + rows[2].w * value.w,
    rows[3].x * value.x + rows[3].y * value.y + rows[3].z * value.z + rows[3].w * value.w
  );
}

)CUDA_CHUNK";

inline const char* sampling =
    R"CUDA_CHUNK(static __forceinline__ __device__ float3 cosineHemisphere(float3 normal, unsigned int& rng) {
  constexpr float pi = 3.14159265359f;
  float r1 = rand(rng);
  float r2 = rand(rng);
  float phi = 2.0f * pi * r1;
  float radius = sqrtf(r2);

  float3 local = make_float3(cosf(phi) * radius, sinf(phi) * radius, sqrtf(fmaxf(0.0f, 1.0f - r2)));
  float3 up = fabsf(normal.z) < 0.999f ? make_float3(0.0f, 0.0f, 1.0f) : make_float3(1.0f, 0.0f, 0.0f);
  float3 tangent = normalize3(cross3(up, normal));
  float3 bitangent = cross3(normal, tangent);
  return normalize3(add3(add3(mul3(tangent, local.x), mul3(bitangent, local.y)), mul3(normal, local.z)));
}

static __forceinline__ __device__ float3 uniformHemisphere(float3 normal, unsigned int& rng) {
  constexpr float pi = 3.14159265359f;
  float r1 = rand(rng);
  float r2 = rand(rng);
  float z = r1;
  float radius = sqrtf(fmaxf(0.0f, 1.0f - z * z));
  float phi = 2.0f * pi * r2;
  float3 local = make_float3(cosf(phi) * radius, sinf(phi) * radius, z);
  float3 up = fabsf(normal.z) < 0.999f ? make_float3(0.0f, 0.0f, 1.0f) : make_float3(1.0f, 0.0f, 0.0f);
  float3 tangent = normalize3(cross3(up, normal));
  float3 bitangent = cross3(normal, tangent);
  return normalize3(add3(add3(mul3(tangent, local.x), mul3(bitangent, local.y)), mul3(normal, local.z)));
}

static __forceinline__ __device__ void makeBasis(float3 normal, float3& tangent, float3& bitangent) {
  float3 up = fabsf(normal.z) < 0.999f ? make_float3(0.0f, 0.0f, 1.0f) : make_float3(1.0f, 0.0f, 0.0f);
  tangent = normalize3(cross3(up, normal));
  bitangent = cross3(normal, tangent);
}

static __forceinline__ __device__ float3 toLocal(float3 value, float3 tangent, float3 bitangent, float3 normal) {
  return make_float3(dot3(value, tangent), dot3(value, bitangent), dot3(value, normal));
}

static __forceinline__ __device__ float3 toWorld(float3 value, float3 tangent, float3 bitangent, float3 normal) {
  return add3(add3(mul3(tangent, value.x), mul3(bitangent, value.y)), mul3(normal, value.z));
}

)CUDA_CHUNK";

inline const char* brdf =
    R"CUDA_CHUNK(static __forceinline__ __device__ float ggxD(float NoH, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float d = (NoH * a2 - NoH) * NoH + 1.0f;
  return a2 / fmaxf(3.14159265359f * d * d, 1e-6f);
}

static __forceinline__ __device__ float smithG1(float NoV, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float b = NoV * NoV;
  return 2.0f * NoV / fmaxf(NoV + sqrtf(a2 + b - a2 * b), 1e-6f);
}

static __forceinline__ __device__ float3 fresnelSchlick(float VoH, float3 f0) {
  float f = powf(fmaxf(1.0f - VoH, 0.0f), 5.0f);
  return add3(f0, mul3(sub3(make_float3(1.0f, 1.0f, 1.0f), f0), f));
}

constexpr float DELTA_ROUGHNESS = 0.01f;
constexpr float MIN_GGX_ROUGHNESS = 0.01f;
constexpr float MIN_GGX_ALPHA = MIN_GGX_ROUGHNESS * MIN_GGX_ROUGHNESS;

static __forceinline__ __device__ bool usesDeltaMirrorLobe(MaterialData material) {
  return params.enableMirrorReflection != 0u && params.materialMode == MATERIAL_PBR_GGX &&
         material.roughness <= DELTA_ROUGHNESS;
}

static __forceinline__ __device__ float3 dielectricFresnel(MaterialData material, float NoV) {
  float metallic = fminf(fmaxf(material.metallic, 0.0f), 1.0f);
  float3 f0 = lerp3(make_float3(0.04f, 0.04f, 0.04f), material.albedo, metallic);
  return fresnelSchlick(NoV, f0);
}

static __forceinline__ __device__ float3 evalDiffuse(MaterialData material, float3 normal, float3 view, float3 lightDir) {
  constexpr float invPi = 0.31830988618f;
  float NoL = fmaxf(dot3(normal, lightDir), 0.0f);
  float NoV = fmaxf(dot3(normal, view), 0.0f);
  if (NoL <= 0.0f || NoV <= 0.0f) {
    return make_float3(0.0f, 0.0f, 0.0f);
  }

  float3 F = dielectricFresnel(material, NoV);
  float3 kd = mul3(sub3(make_float3(1.0f, 1.0f, 1.0f), F), 1.0f - material.metallic);
  return mul3(mul3(kd, material.albedo), invPi);
}

static __forceinline__ __device__ float diffusePdf(float3 normal, float3 lightDir) {
  return fmaxf(dot3(normal, lightDir), 0.0f) * 0.31830988618f;
}

static __forceinline__ __device__ float uniformHemispherePdf(float3 normal, float3 lightDir) {
  return dot3(normal, lightDir) > 0.0f ? 0.15915494309f : 0.0f;
}

static __forceinline__ __device__ float3 evalPbr(MaterialData material, float3 normal, float3 view, float3 lightDir) {
  constexpr float invPi = 0.31830988618f;
  float NoL = fmaxf(dot3(normal, lightDir), 0.0f);
  float NoV = fmaxf(dot3(normal, view), 0.0f);
  if (NoL <= 0.0f || NoV <= 0.0f) {
    return make_float3(0.0f, 0.0f, 0.0f);
  }

  float3 halfVector = normalize3(add3(view, lightDir));
  float NoH = fmaxf(dot3(normal, halfVector), 0.0f);
  float VoH = fmaxf(dot3(view, halfVector), 0.0f);
  float roughness = fmaxf(material.roughness, MIN_GGX_ROUGHNESS);
  float metallic = fminf(fmaxf(material.metallic, 0.0f), 1.0f);
  float3 f0 = lerp3(make_float3(0.04f, 0.04f, 0.04f), material.albedo, metallic);
  float3 F = fresnelSchlick(VoH, f0);
  float D = ggxD(NoH, roughness);
  float G = smithG1(NoV, roughness) * smithG1(NoL, roughness);
  float3 spec = mul3(F, D * G / fmaxf(4.0f * NoV * NoL, 1e-6f));
  float3 kd = mul3(sub3(make_float3(1.0f, 1.0f, 1.0f), F), 1.0f - metallic);
  float3 diffuse = mul3(mul3(kd, material.albedo), invPi);
  return add3(diffuse, spec);
}

static __forceinline__ __device__ float pbrPdf(MaterialData material, float3 normal, float3 view, float3 lightDir) {
  float NoL = fmaxf(dot3(normal, lightDir), 0.0f);
  float NoV = fmaxf(dot3(normal, view), 0.0f);
  if (NoL <= 0.0f || NoV <= 0.0f) {
    return 0.0f;
  }
  float3 halfVector = normalize3(add3(view, lightDir));
  float NoH = fmaxf(dot3(normal, halfVector), 0.0f);
  float VoH = fmaxf(dot3(view, halfVector), 0.0f);
  float specPdf = ggxD(NoH, fmaxf(material.roughness, MIN_GGX_ROUGHNESS)) * NoH / fmaxf(4.0f * VoH, 1e-6f);
  float diffusePdf = NoL * 0.31830988618f;
  float specWeight = fminf(fmaxf(material.metallic + luminance(material.albedo) * 0.25f, 0.1f), 0.9f);
  return specPdf * specWeight + diffusePdf * (1.0f - specWeight);
}

static __forceinline__ __device__ float specularLobeProbability(MaterialData material) {
  if (params.materialMode == MATERIAL_LAMBERT || params.samplingMode == SAMPLING_UNIFORM ||
      params.samplingMode == SAMPLING_COSINE) {
    return 0.0f;
  }
  return fminf(fmaxf(material.metallic + luminance(material.albedo) * 0.25f, 0.1f), 0.9f);
}

static __forceinline__ __device__ float3 sampleGgxVndf(float3 viewLocal, float roughness, float u1, float u2) {
  float alpha = fmaxf(roughness * roughness, MIN_GGX_ALPHA);
  float3 Vh = normalize3(make_float3(alpha * viewLocal.x, alpha * viewLocal.y, viewLocal.z));
  float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
  float3 T1 = lensq > 0.0f ? mul3(make_float3(-Vh.y, Vh.x, 0.0f), rsqrtf(lensq)) : make_float3(1.0f, 0.0f, 0.0f);
  float3 T2 = cross3(Vh, T1);
  float r = sqrtf(u1);
  float phi = 6.28318530718f * u2;
  float t1 = r * cosf(phi);
  float t2 = r * sinf(phi);
  float s = 0.5f * (1.0f + Vh.z);
  t2 = (1.0f - s) * sqrtf(fmaxf(0.0f, 1.0f - t1 * t1)) + s * t2;
  float3 Nh = add3(add3(mul3(T1, t1), mul3(T2, t2)),
                   mul3(Vh, sqrtf(fmaxf(0.0f, 1.0f - t1 * t1 - t2 * t2))));
  return normalize3(make_float3(alpha * Nh.x, alpha * Nh.y, fmaxf(0.0f, Nh.z)));
}

static __forceinline__ __device__ float powerHeuristic(float a, float b) {
  float a2 = a * a;
  float b2 = b * b;
  return a2 / fmaxf(a2 + b2, 1e-8f);
}

static __forceinline__ __device__ float balanceHeuristic(float a, float b) {
  return a / fmaxf(a + b, 1e-8f);
}

static __forceinline__ __device__ float misWeight(float sampledPdf, float otherPdf) {
  if (params.misMode == MIS_BALANCE) {
    return balanceHeuristic(sampledPdf, otherPdf);
  }
  if (params.misMode == MIS_POWER) {
    return powerHeuristic(sampledPdf, otherPdf);
  }
  return 1.0f;
}

static __forceinline__ __device__ float triangleArea(unsigned int primitiveIndex) {
  uint3 tri = params.indices[primitiveIndex];
  float3 v0 = params.vertices[tri.x];
  float3 v1 = params.vertices[tri.y];
  float3 v2 = params.vertices[tri.z];
  float3 cross = cross3(sub3(v1, v0), sub3(v2, v0));
  return 0.5f * sqrtf(dot3(cross, cross));
}

static __forceinline__ __device__ float lightPdfForHit(unsigned int primitiveIndex, float3 incomingDirection, float distance) {
  if (params.lightCount == 0u) {
    return 0.0f;
  }

  MaterialData material = params.materials[params.materialIndices[primitiveIndex]];
  if (dot3(material.emission, material.emission) <= 0.0f) {
    return 0.0f;
  }

  uint3 tri = params.indices[primitiveIndex];
  float3 v0 = params.vertices[tri.x];
  float3 v1 = params.vertices[tri.y];
  float3 v2 = params.vertices[tri.z];
  float3 lightNormal = normalize3(cross3(sub3(v1, v0), sub3(v2, v0)));
  float lightCos = fmaxf(dot3(lightNormal, mul3(incomingDirection, -1.0f)), 0.0f);
  float area = triangleArea(primitiveIndex);
  return (distance * distance) / fmaxf(lightCos * area * float(params.lightCount), 1e-6f);
}

static __forceinline__ __device__ float materialPdf(MaterialData material, float3 normal, float3 view, float3 direction) {
  if (usesDeltaMirrorLobe(material)) {
    float metallic = fminf(fmaxf(material.metallic, 0.0f), 1.0f);
    if (metallic >= 0.95f) {
      return 0.0f;
    }

    float NoV = fmaxf(dot3(normal, view), 0.0f);
    float3 F = fresnelSchlick(NoV, make_float3(0.04f, 0.04f, 0.04f));
    float specProbability = fminf(fmaxf(luminance(F), 0.02f), 0.95f);
    return diffusePdf(normal, direction) * fmaxf(1.0f - specProbability, 0.0f);
  }

  if (params.samplingMode == SAMPLING_UNIFORM) {
    return uniformHemispherePdf(normal, direction);
  }
  if (params.materialMode == MATERIAL_LAMBERT || params.samplingMode == SAMPLING_COSINE) {
    return diffusePdf(normal, direction);
  }
  return pbrPdf(material, normal, view, direction);
}

static __forceinline__ __device__ float3 evalMaterial(MaterialData material, float3 normal, float3 view, float3 direction) {
  if (usesDeltaMirrorLobe(material)) {
    if (material.metallic >= 0.95f) {
      return make_float3(0.0f, 0.0f, 0.0f);
    }
    return evalDiffuse(material, normal, view, direction);
  }

  if (params.materialMode == MATERIAL_LAMBERT) {
    return evalDiffuse(material, normal, view, direction);
  }
  return evalPbr(material, normal, view, direction);
}

struct BsdfSample {
  float3 direction;
  float3 weight;
  float pdf;
  bool isDelta;
};

static __forceinline__ __device__ BsdfSample sampleDiffuseLobe(MaterialData material, float3 normal, float3 view,
                                                               unsigned int& rng, float lobeProbability) {
  BsdfSample sample;
  sample.direction = params.samplingMode == SAMPLING_UNIFORM ? uniformHemisphere(normal, rng)
                                                             : cosineHemisphere(normal, rng);
  float NoL = fmaxf(dot3(normal, sample.direction), 0.0f);
  float lobePdf = params.samplingMode == SAMPLING_UNIFORM ? uniformHemispherePdf(normal, sample.direction)
                                                          : diffusePdf(normal, sample.direction);
  sample.pdf = lobePdf * lobeProbability;
  sample.weight = mul3(evalDiffuse(material, normal, view, sample.direction),
                       NoL / fmaxf(sample.pdf, 1e-6f));
  sample.isDelta = false;
  return sample;
}

static __forceinline__ __device__ BsdfSample sampleBsdf(MaterialData material, float3 normal, float3 view,
                                                        unsigned int& rng) {
  BsdfSample sample;
  sample.direction = normal;
  sample.weight = make_float3(0.0f, 0.0f, 0.0f);
  sample.pdf = 0.0f;
  sample.isDelta = false;

  float NoV = fmaxf(dot3(normal, view), 0.0f);
  float metallic = fminf(fmaxf(material.metallic, 0.0f), 1.0f);
  float3 f0 = lerp3(make_float3(0.04f, 0.04f, 0.04f), material.albedo, metallic);
  float3 F = fresnelSchlick(NoV, f0);

  if (usesDeltaMirrorLobe(material)) {
    float specProbability = metallic >= 0.95f ? 1.0f : fminf(fmaxf(luminance(F), 0.02f), 0.95f);
    if (rand(rng) < specProbability) {
      sample.direction = normalize3(reflect3(mul3(view, -1.0f), normal));
      sample.weight = div3(F, specProbability);
      sample.pdf = specProbability;
      sample.isDelta = true;
      return sample;
    }

    float diffuseProbability = fmaxf(1.0f - specProbability, 1e-6f);
    return sampleDiffuseLobe(material, normal, view, rng, diffuseProbability);
  }

  float specProbability = specularLobeProbability(material);
  if (rand(rng) < specProbability) {
    float3 tangent;
    float3 bitangent;
    makeBasis(normal, tangent, bitangent);
    float3 halfLocal = sampleGgxVndf(toLocal(view, tangent, bitangent, normal),
                                     material.roughness, rand(rng), rand(rng));
    float3 halfWorld = toWorld(halfLocal, tangent, bitangent, normal);
    sample.direction = normalize3(sub3(mul3(halfWorld, 2.0f * dot3(view, halfWorld)), view));
  } else {
    float diffuseProbability = fmaxf(1.0f - specProbability, 1e-6f);
    if (params.materialMode == MATERIAL_LAMBERT || specProbability <= 0.0f) {
      diffuseProbability = 1.0f;
    }
    return sampleDiffuseLobe(material, normal, view, rng, diffuseProbability);
  }

  float NoL = fmaxf(dot3(normal, sample.direction), 0.0f);
  sample.pdf = pbrPdf(material, normal, view, sample.direction);
  sample.weight = mul3(evalPbr(material, normal, view, sample.direction),
                       NoL / fmaxf(sample.pdf, 1e-6f));
  return sample;
}

)CUDA_CHUNK";

inline const char* raygen = R"CUDA_CHUNK(extern "C" __global__ void __raygen__rg() {
  const uint3 index = optixGetLaunchIndex();
  const unsigned int pixelIndex = index.y * params.width + index.x;

  float3 color = make_float3(0.0f, 0.0f, 0.0f);
  float3 directColor = make_float3(0.0f, 0.0f, 0.0f);
  float3 firstNormal = make_float3(0.0f, 0.0f, 0.0f);
  float3 firstAlbedo = make_float3(0.0f, 0.0f, 0.0f);
  float firstDepth = 0.0f;
  float lastPdf = 0.0f;
  float lastMisWeight = 0.0f;
  unsigned int maxBounceSeen = 0u;
  unsigned int spp = max(params.samplesPerPixel, 1u);

  for (unsigned int sample = 0; sample < spp; ++sample) {
    unsigned int rng = initRng(pixelIndex, sample, params.frameIndex);
    float2 jitter = make_float2(rand(rng), rand(rng));
    float2 uv = make_float2((float(index.x) + jitter.x) / float(params.width),
                            (float(index.y) + jitter.y) / float(params.height));
    float4 clip = make_float4(uv.x * 2.0f - 1.0f, uv.y * 2.0f - 1.0f, 1.0f, 1.0f);
    float4 world = mulMat(params.invViewProjection, clip);
    world.x /= world.w;
    world.y /= world.w;
    world.z /= world.w;

    float3 origin = params.cameraPosition;
    float3 direction = normalize3(sub3(make_float3(world.x, world.y, world.z), origin));
    float3 throughput = make_float3(1.0f, 1.0f, 1.0f);
    float previousPdf = 0.0f;
    bool previousDelta = true;

    for (unsigned int bounce = 0; bounce < params.maxBounces; ++bounce) {
      maxBounceSeen = max(maxBounceSeen, bounce);
      unsigned int tPayload = 0;
      unsigned int nxPayload = 0;
      unsigned int nyPayload = 0;
      unsigned int nzPayload = 0;
      unsigned int arPayload = 0;
      unsigned int agPayload = 0;
      unsigned int abPayload = 0;
      unsigned int erPayload = 0;
      unsigned int egPayload = 0;
      unsigned int ebPayload = 0;
      unsigned int roughPayload = 0;
      unsigned int metalPayload = 0;
      unsigned int primitivePayload = 0;

      optixTrace(params.traversable, origin, direction, 0.001f, 1e20f, 0.0f,
                 OptixVisibilityMask(255), OPTIX_RAY_FLAG_NONE, 0, 1, 0,
                 tPayload, nxPayload, nyPayload, nzPayload, arPayload, agPayload, abPayload,
                 erPayload, egPayload, ebPayload, roughPayload, metalPayload, primitivePayload);

      float t = uintToFloat(tPayload);
      if (t < 0.0f) {
        color = add3(color, mul3(throughput, sampleEnvironment(direction)));
        break;
      }

      float3 normal = normalize3(make_float3(uintToFloat(nxPayload), uintToFloat(nyPayload), uintToFloat(nzPayload)));
      float3 albedo = make_float3(uintToFloat(arPayload), uintToFloat(agPayload), uintToFloat(abPayload));
      float3 emission = params.enableEmissiveLights != 0u
                            ? make_float3(uintToFloat(erPayload), uintToFloat(egPayload), uintToFloat(ebPayload))
                            : make_float3(0.0f, 0.0f, 0.0f);
      MaterialData material;
      material.albedo = albedo;
      material.emission = emission;
      material.roughness = uintToFloat(roughPayload);
      material.metallic = params.materialMode == MATERIAL_LAMBERT ? 0.0f : uintToFloat(metalPayload);

      if (bounce == 0u) {
        firstNormal = normal;
        firstAlbedo = albedo;
        firstDepth = t;
      }

      if (params.integratorMode == INTEGRATOR_FIRST_HIT) {
        float NoL = fmaxf(dot3(normal, normalize3(make_float3(0.4f, 0.8f, 0.2f))), 0.0f);
        color = add3(mul3(albedo, 0.15f + NoL * 0.85f), emission);
        break;
      }

      float emissionWeight = 1.0f;
      if (bounce > 0u && !previousDelta && params.enableDirectLighting != 0u) {
        if (params.integratorMode == INTEGRATOR_DIRECT_LIGHTING) {
          emissionWeight = 0.0f;
        } else if (params.integratorMode == INTEGRATOR_MIS && previousPdf > 0.0f) {
          float lightPdf = lightPdfForHit(primitivePayload, direction, t);
          emissionWeight = lightPdf > 0.0f ? misWeight(previousPdf, lightPdf) : 1.0f;
        }
      }

      color = add3(color, mul3(throughput, mul3(emission, emissionWeight)));
      if (dot3(emission, emission) > 0.0f) {
        break;
      }

      origin = add3(origin, mul3(direction, t));
      origin = add3(origin, mul3(normal, 0.001f));
      float3 view = mul3(direction, -1.0f);

      bool useDirectLighting = params.integratorMode >= INTEGRATOR_DIRECT_LIGHTING &&
                               params.enableDirectLighting != 0u &&
                               params.enableEmissiveLights != 0u;
      if (useDirectLighting && params.lightCount > 0u) {
        unsigned int lightIndex = min((unsigned int)(rand(rng) * float(params.lightCount)), params.lightCount - 1u);
        TriangleLight light = params.lights[lightIndex];
        float r1 = rand(rng);
        float r2 = rand(rng);
        float su0 = sqrtf(r1);
        float b0 = 1.0f - su0;
        float b1 = r2 * su0;
        float b2 = 1.0f - b0 - b1;
        float3 lightPos = add3(add3(mul3(light.v0, b0), mul3(light.v1, b1)), mul3(light.v2, b2));
        float3 toLight = sub3(lightPos, origin);
        float dist2 = dot3(toLight, toLight);
        float dist = sqrtf(dist2);
        float3 lightDir = div3(toLight, dist);
        float NoL = fmaxf(dot3(normal, lightDir), 0.0f);
        float lightCos = fmaxf(dot3(light.normal, mul3(lightDir, -1.0f)), 0.0f);
        if (NoL > 0.0f && lightCos > 0.0f) {
          bool visible = true;
          if (params.enableShadowRays != 0u) {
            unsigned int shadowPayload = 0;
            optixTrace(params.traversable, origin, lightDir, 0.001f, fmaxf(dist - 0.002f, 0.001f), 0.0f,
                       OptixVisibilityMask(255), OPTIX_RAY_FLAG_NONE, 0, 1, 0,
                       shadowPayload, nxPayload, nyPayload, nzPayload, arPayload, agPayload, abPayload,
                       erPayload, egPayload, ebPayload, roughPayload, metalPayload, primitivePayload);
            visible = uintToFloat(shadowPayload) < 0.0f;
          }
          if (visible) {
            MaterialData lightMaterial = params.materials[light.materialIndex];
            float lightPdf = dist2 / fmaxf(lightCos * light.area * float(params.lightCount), 1e-6f);
            float bsdfPdf = materialPdf(material, normal, view, lightDir);
            float weight = params.integratorMode == INTEGRATOR_MIS ? misWeight(lightPdf, bsdfPdf) : 1.0f;
            float3 bsdf = evalMaterial(material, normal, view, lightDir);
            float3 contribution = mul3(mul3(throughput, mul3(bsdf, lightMaterial.emission)),
                                       NoL * weight / fmaxf(lightPdf, 1e-6f));
            color = add3(color, contribution);
            directColor = add3(directColor, contribution);
            lastMisWeight = weight;
          }
        }
      }

      BsdfSample bsdfSample = sampleBsdf(material, normal, view, rng);
      if (bsdfSample.pdf <= 0.0f || luminance(bsdfSample.weight) <= 0.0f) {
        break;
      }
      direction = bsdfSample.direction;
      throughput = mul3(throughput, bsdfSample.weight);
      previousPdf = bsdfSample.pdf;
      previousDelta = bsdfSample.isDelta;
      lastPdf = bsdfSample.pdf;

      if (params.enableRussianRoulette != 0u && bounce >= 3u) {
        float continueProbability = fminf(fmaxf(luminance(throughput), 0.05f), 0.95f);
        if (rand(rng) > continueProbability) {
          break;
        }
        throughput = div3(throughput, continueProbability);
      }
    }
  }

  color = mul3(color, 1.0f / float(spp));
  directColor = mul3(directColor, 1.0f / float(spp));

  if (params.debugView == DEBUG_NORMALS) {
    color = add3(mul3(firstNormal, 0.5f), make_float3(0.5f, 0.5f, 0.5f));
  } else if (params.debugView == DEBUG_ALBEDO) {
    color = firstAlbedo;
  } else if (params.debugView == DEBUG_DEPTH) {
    float depth = 1.0f / (1.0f + firstDepth * 0.05f);
    color = make_float3(depth, depth, depth);
  } else if (params.debugView == DEBUG_DIRECT) {
    color = directColor;
  } else if (params.debugView == DEBUG_INDIRECT) {
    color = max3(sub3(color, directColor), 0.0f);
  } else if (params.debugView == DEBUG_PDF) {
    float pdf = fminf(lastPdf * 4.0f, 1.0f);
    color = make_float3(pdf, pdf, pdf);
  } else if (params.debugView == DEBUG_MIS_WEIGHT) {
    color = make_float3(lastMisWeight, lastMisWeight, lastMisWeight);
  } else if (params.debugView == DEBUG_BOUNCE_COUNT) {
    float bounces = float(maxBounceSeen) / fmaxf(float(params.maxBounces), 1.0f);
    color = make_float3(bounces, bounces, bounces);
  }

  if (params.debugView == DEBUG_BEAUTY && params.accumulate != 0u && params.frameIndex > 0u) {
    float4 previous = params.accumulation[pixelIndex];
    color = add3(mul3(make_float3(previous.x, previous.y, previous.z), float(params.frameIndex)),
                 color);
    color = mul3(color, 1.0f / float(params.frameIndex + 1u));
  }
  if (params.accumulate != 0u) {
    params.accumulation[pixelIndex] = make_float4(color.x, color.y, color.z, 1.0f);
  }
  params.output[pixelIndex] = make_half4(__float2half(color.x), __float2half(color.y), __float2half(color.z), __float2half(1.0f));
}

)CUDA_CHUNK";

inline const char* hit_programs = R"CUDA_CHUNK(extern "C" __global__ void __miss__ms() {
  optixSetPayload_0(floatToUint(-1.0f));
}

extern "C" __global__ void __anyhit__ah() {
  unsigned int primitiveIndex = optixGetPrimitiveIndex();
  uint3 tri = params.indices[primitiveIndex];
  float2 bary = optixGetTriangleBarycentrics();
  float2 uv0 = params.uvs[tri.x];
  float2 uv1 = params.uvs[tri.y];
  float2 uv2 = params.uvs[tri.z];
  float2 uv = make_float2(uv0.x * (1.0f - bary.x - bary.y) + uv1.x * bary.x + uv2.x * bary.y,
                          uv0.y * (1.0f - bary.x - bary.y) + uv1.y * bary.x + uv2.y * bary.y);
  MaterialSample sample = sampleMaterial(params.materials[params.materialIndices[primitiveIndex]], uv);

  if (params.enableAlpha == 0u) {
    return;
  }

  if (params.materials[params.materialIndices[primitiveIndex]].alphaTest >= 0.0f &&
      sample.opacity < params.materials[params.materialIndices[primitiveIndex]].alphaTest) {
    optixIgnoreIntersection();
  }

  if (sample.opacity >= 0.999f) {
    return;
  }

  float3 origin = optixGetWorldRayOrigin();
  float3 direction = optixGetWorldRayDirection();
  unsigned int seed = hashRay(origin, direction, primitiveIndex);
  if (rand(seed) > sample.opacity) {
    optixIgnoreIntersection();
  }
}

extern "C" __global__ void __closesthit__ch() {
  unsigned int primitiveIndex = optixGetPrimitiveIndex();
  uint3 tri = params.indices[primitiveIndex];
  float2 bary = optixGetTriangleBarycentrics();

  float3 n0 = params.normals[tri.x];
  float3 n1 = params.normals[tri.y];
  float3 n2 = params.normals[tri.z];
  float3 normal = normalize3(add3(add3(mul3(n0, 1.0f - bary.x - bary.y), mul3(n1, bary.x)), mul3(n2, bary.y)));
  float2 uv0 = params.uvs[tri.x];
  float2 uv1 = params.uvs[tri.y];
  float2 uv2 = params.uvs[tri.z];
  float2 uv = make_float2(uv0.x * (1.0f - bary.x - bary.y) + uv1.x * bary.x + uv2.x * bary.y,
                          uv0.y * (1.0f - bary.x - bary.y) + uv1.y * bary.x + uv2.y * bary.y);

  MaterialData material = params.materials[params.materialIndices[primitiveIndex]];
  MaterialSample sample = sampleMaterial(material, uv);
  if (params.enableNormalMaps != 0u && material.hasNormalMap != 0u) {
    float3 p0 = params.vertices[tri.x];
    float3 p1 = params.vertices[tri.y];
    float3 p2 = params.vertices[tri.z];
    float3 edge1 = sub3(p1, p0);
    float3 edge2 = sub3(p2, p0);
    float2 duv1 = make_float2(uv1.x - uv0.x, uv1.y - uv0.y);
    float2 duv2 = make_float2(uv2.x - uv0.x, uv2.y - uv0.y);
    float det = duv1.x * duv2.y - duv2.x * duv1.y;
    if (fabsf(det) > 1e-8f) {
      float invDet = 1.0f / det;
      float3 tangent = normalize3(mul3(sub3(mul3(edge1, duv2.y), mul3(edge2, duv1.y)), invDet));
      float3 bitangent = normalize3(mul3(sub3(mul3(edge2, duv1.x), mul3(edge1, duv2.x)), invDet));
      normal = normalize3(add3(add3(mul3(tangent, sample.tangentNormal.x),
                                    mul3(bitangent, sample.tangentNormal.y)),
                               mul3(normal, sample.tangentNormal.z)));
    }
  }

  optixSetPayload_0(floatToUint(optixGetRayTmax()));
  optixSetPayload_1(floatToUint(normal.x));
  optixSetPayload_2(floatToUint(normal.y));
  optixSetPayload_3(floatToUint(normal.z));
  optixSetPayload_4(floatToUint(sample.albedo.x));
  optixSetPayload_5(floatToUint(sample.albedo.y));
  optixSetPayload_6(floatToUint(sample.albedo.z));
  optixSetPayload_7(floatToUint(sample.emission.x));
  optixSetPayload_8(floatToUint(sample.emission.y));
  optixSetPayload_9(floatToUint(sample.emission.z));
  optixSetPayload_10(floatToUint(sample.roughness));
  optixSetPayload_11(floatToUint(sample.metallic));
  optixSetPayload_12(primitiveIndex);
})CUDA_CHUNK";

inline std::string buildPathTracerShaderSource() {
  std::string source;
  source.reserve(26331);
  source += types_and_math;
  source += material_sampling;
  source += environment;
  source += sampling;
  source += brdf;
  source += raygen;
  source += hit_programs;
  return source;
}

} // namespace blkhurst::optix::shaders
