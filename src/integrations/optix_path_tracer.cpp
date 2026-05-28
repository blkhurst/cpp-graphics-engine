#include <blkhurst/integrations/optix_path_tracer.hpp>

#ifdef BLKHURST_ENABLE_OPTIX

#include <blkhurst/cameras/camera.hpp>
#include <blkhurst/geometry/geometry.hpp>
#include <blkhurst/materials/pbr_material.hpp>
#include <blkhurst/objects/mesh.hpp>
#include <blkhurst/scene/scene.hpp>
#include <blkhurst/textures/texture.hpp>

// clang-format off
// GLAD must be included before CUDA's OpenGL interop header, which includes system gl.h.
#include <glad/gl.h>
#include <cuda_gl_interop.h>
#include <cuda_runtime.h>
#include <nvrtc.h>
#include <optix.h>
#include <optix_stubs.h>
// clang-format on
#include <spdlog/spdlog.h>

#include "optix/shaders/path_tracer_shader.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::size_t kRgba16FBytesPerPixel = sizeof(std::uint16_t) * 4;
constexpr int kSbtAlignment = OPTIX_SBT_RECORD_ALIGNMENT;

// Path tracer CUDA source is split into readable chunks under optix/shaders.

void checkCuda(cudaError_t result, const char* label) {
  if (result != cudaSuccess) {
    throw std::runtime_error(std::string(label) + ": " + cudaGetErrorString(result));
  }
}

void checkNvrtc(nvrtcResult result, const char* label) {
  if (result != NVRTC_SUCCESS) {
    throw std::runtime_error(std::string(label) + ": " + nvrtcGetErrorString(result));
  }
}

void checkOptix(OptixResult result, const char* label) {
  if (result != OPTIX_SUCCESS) {
    throw std::runtime_error(std::string(label) + ": " + optixGetErrorName(result) + " (" +
                             std::to_string(static_cast<int>(result)) + ") - " +
                             optixGetErrorString(result));
  }
}

struct MaterialData {
  float3 albedo;
  float3 emission;
  float metallic = 0.0F;
  float roughness = 1.0F;
  float opacity = 1.0F;
  float alphaTest = -1.0F;
  float normalScale = 1.0F;
  cudaTextureObject_t albedoMap = 0;
  cudaTextureObject_t alphaMap = 0;
  cudaTextureObject_t normalMap = 0;
  cudaTextureObject_t metalnessMap = 0;
  cudaTextureObject_t roughnessMap = 0;
  cudaTextureObject_t aoMap = 0;
  cudaTextureObject_t emissiveMap = 0;
  unsigned hasAlbedoMap = 0;
  unsigned hasAlphaMap = 0;
  unsigned hasNormalMap = 0;
  unsigned hasMetalnessMap = 0;
  unsigned hasRoughnessMap = 0;
  unsigned hasAoMap = 0;
  unsigned hasEmissiveMap = 0;
};

struct MaterialTextures {
  std::shared_ptr<blkhurst::Texture> albedoMap;
  std::shared_ptr<blkhurst::Texture> alphaMap;
  std::shared_ptr<blkhurst::Texture> normalMap;
  std::shared_ptr<blkhurst::Texture> metalnessMap;
  std::shared_ptr<blkhurst::Texture> roughnessMap;
  std::shared_ptr<blkhurst::Texture> aoMap;
  std::shared_ptr<blkhurst::Texture> emissiveMap;
};

struct TriangleLight {
  float3 v0;
  float3 v1;
  float3 v2;
  float3 normal;
  float area = 0.0F;
  unsigned materialIndex = 0;
};

struct LaunchParams {
  CUdeviceptr output = 0;
  CUdeviceptr accumulation = 0;
  unsigned width = 0;
  unsigned height = 0;
  unsigned samplesPerPixel = 1;
  unsigned maxBounces = 5;
  unsigned frameIndex = 0;
  unsigned accumulate = 1;
  unsigned integratorMode = static_cast<unsigned>(blkhurst::OptixIntegratorMode::Mis);
  unsigned samplingMode = static_cast<unsigned>(blkhurst::OptixSamplingMode::GgxVndf);
  unsigned environmentMode = static_cast<unsigned>(blkhurst::OptixEnvironmentMode::Hdri);
  unsigned materialMode = static_cast<unsigned>(blkhurst::OptixMaterialMode::PbrGgx);
  unsigned misMode = static_cast<unsigned>(blkhurst::OptixMisMode::Power);
  unsigned debugView = static_cast<unsigned>(blkhurst::OptixDebugView::Beauty);
  unsigned enableTextures = 1;
  unsigned enableNormalMaps = 1;
  unsigned enableAlpha = 1;
  unsigned enableMirrorReflection = 1;
  unsigned enableDirectLighting = 1;
  unsigned enableShadowRays = 1;
  unsigned enableEmissiveLights = 1;
  unsigned enableRussianRoulette = 0;
  OptixTraversableHandle traversable = 0;
  float3 cameraPosition{};
  float4 invViewProjection[4]{};
  CUdeviceptr vertices = 0;
  CUdeviceptr uvs = 0;
  CUdeviceptr normals = 0;
  CUdeviceptr indices = 0;
  CUdeviceptr materialIndices = 0;
  CUdeviceptr materials = 0;
  CUdeviceptr lights = 0;
  unsigned lightCount = 0;
  cudaTextureObject_t environmentMap = 0;
  unsigned hasEnvironmentMap = 0;
  float3 environmentColor = make_float3(0.02F, 0.025F, 0.035F);
  float environmentIntensity = 1.0F;
};

struct RegisteredTexture {
  const blkhurst::Texture* texture = nullptr;
  cudaGraphicsResource* resource = nullptr;
  cudaTextureObject_t object = 0;
};

template <typename T> struct SbtRecord {
  alignas(kSbtAlignment) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
  T data;
};

struct EmptySbtData {};
using EmptySbtRecord = SbtRecord<EmptySbtData>;

std::string compilePtx() {
  nvrtcProgram program = nullptr;
  const std::string cudaSource = blkhurst::optix::shaders::buildPathTracerShaderSource();
  checkNvrtc(
      nvrtcCreateProgram(&program, cudaSource.c_str(), "optix_path_tracer.cu", 0, nullptr, nullptr),
      "nvrtcCreateProgram");

  int device = 0;
  cudaDeviceProp props{};
  checkCuda(cudaGetDevice(&device), "cudaGetDevice");
  checkCuda(cudaGetDeviceProperties(&props, device), "cudaGetDeviceProperties");
  const std::string arch =
      "--gpu-architecture=compute_" + std::to_string(props.major) + std::to_string(props.minor);
  const std::string optixInclude = std::string("-I") + BLKHURST_OPTIX_INCLUDE_DIR;
  const char* cudaInclude = "-I/usr/include";

  const std::array<const char*, 6> options{
      "--std=c++17", "--use_fast_math",    "--device-as-default-execution-space",
      arch.c_str(),  optixInclude.c_str(), cudaInclude,
  };

  const nvrtcResult compileResult =
      nvrtcCompileProgram(program, static_cast<int>(options.size()), options.data());

  std::size_t logSize = 0;
  checkNvrtc(nvrtcGetProgramLogSize(program, &logSize), "nvrtcGetProgramLogSize");
  if (logSize > 1) {
    std::string log(logSize, '\0');
    checkNvrtc(nvrtcGetProgramLog(program, log.data()), "nvrtcGetProgramLog");
    spdlog::debug("OptixPathTracer NVRTC log:\n{}", log);
  }
  checkNvrtc(compileResult, "nvrtcCompileProgram");

  std::size_t ptxSize = 0;
  checkNvrtc(nvrtcGetPTXSize(program, &ptxSize), "nvrtcGetPTXSize");
  std::string ptx(ptxSize, '\0');
  checkNvrtc(nvrtcGetPTX(program, ptx.data()), "nvrtcGetPTX");
  checkNvrtc(nvrtcDestroyProgram(&program), "nvrtcDestroyProgram");
  return ptx;
}

float3 makeFloat3(const glm::vec3& value) {
  return make_float3(value.x, value.y, value.z);
}

float4 makeFloat4(const glm::vec4& value) {
  return make_float4(value.x, value.y, value.z, value.w);
}

void upload(CUdeviceptr& ptr, const void* data, std::size_t bytes, const char* label) {
  if (ptr != 0) {
    cudaFree(reinterpret_cast<void*>(ptr));
    ptr = 0;
  }
  if (bytes == 0) {
    return;
  }
  checkCuda(cudaMalloc(reinterpret_cast<void**>(&ptr), bytes), label);
  checkCuda(cudaMemcpy(reinterpret_cast<void*>(ptr), data, bytes, cudaMemcpyHostToDevice), label);
}

} // namespace

namespace blkhurst {

struct OptixPathTracer::Impl {
  OptixPathTracerDesc desc{};
  Object3D* scene = nullptr;
  Camera* camera = nullptr;
  int width = 0;
  int height = 0;
  unsigned frameIndex = 0;
  bool accumulate = true;
  bool sceneDirty = true;

  CUcontext cudaContext = nullptr;
  CUstream stream = nullptr;
  OptixDeviceContext optixContext = nullptr;
  OptixModule module = nullptr;
  OptixProgramGroup raygenGroup = nullptr;
  OptixProgramGroup missGroup = nullptr;
  OptixProgramGroup hitGroup = nullptr;
  OptixPipeline pipeline = nullptr;
  OptixShaderBindingTable sbt{};
  CUdeviceptr raygenRecord = 0;
  CUdeviceptr missRecord = 0;
  CUdeviceptr hitRecord = 0;
  CUdeviceptr paramsBuffer = 0;

  std::vector<float3> vertices;
  std::vector<float2> uvs;
  std::vector<float3> normals;
  std::vector<uint3> indices;
  std::vector<unsigned> materialIndices;
  std::vector<MaterialData> materials;
  std::vector<MaterialTextures> materialTextures;
  std::vector<std::shared_ptr<Material>> sceneMaterials;
  std::vector<TriangleLight> lights;

  CUdeviceptr dVertices = 0;
  CUdeviceptr dUvs = 0;
  CUdeviceptr dNormals = 0;
  CUdeviceptr dIndices = 0;
  CUdeviceptr dMaterialIndices = 0;
  CUdeviceptr dMaterials = 0;
  CUdeviceptr dLights = 0;
  CUdeviceptr gasBuffer = 0;
  OptixTraversableHandle gasHandle = 0;

  CUdeviceptr outputImage = 0;
  CUdeviceptr accumulationImage = 0;
  std::size_t outputBytes = 0;
  cudaGraphicsResource* outputResource = nullptr;
  unsigned outputTextureId = 0;
  std::unordered_map<unsigned, RegisteredTexture> textureResources;
  std::vector<cudaGraphicsResource*> mappedResources;

  explicit Impl(const OptixPathTracerDesc& initialDesc)
      : desc(initialDesc) {
    checkCuda(cudaFree(nullptr), "cudaFree");
    checkOptix(optixInit(), "optixInit");
    checkOptix(optixDeviceContextCreate(cudaContext, nullptr, &optixContext),
               "optixDeviceContextCreate");
    checkCuda(cudaStreamCreate(reinterpret_cast<cudaStream_t*>(&stream)), "cudaStreamCreate");
    createPipeline();
    createSbt();
  }

  ~Impl() {
    unregisterTextures();
    unregisterOutput();
    freeDeviceScene();
    if (outputImage != 0) {
      cudaFree(reinterpret_cast<void*>(outputImage));
    }
    if (accumulationImage != 0) {
      cudaFree(reinterpret_cast<void*>(accumulationImage));
    }
    if (paramsBuffer != 0) {
      cudaFree(reinterpret_cast<void*>(paramsBuffer));
    }
    if (raygenRecord != 0) {
      cudaFree(reinterpret_cast<void*>(raygenRecord));
    }
    if (missRecord != 0) {
      cudaFree(reinterpret_cast<void*>(missRecord));
    }
    if (hitRecord != 0) {
      cudaFree(reinterpret_cast<void*>(hitRecord));
    }
    if (pipeline != nullptr) {
      optixPipelineDestroy(pipeline);
    }
    if (raygenGroup != nullptr) {
      optixProgramGroupDestroy(raygenGroup);
    }
    if (missGroup != nullptr) {
      optixProgramGroupDestroy(missGroup);
    }
    if (hitGroup != nullptr) {
      optixProgramGroupDestroy(hitGroup);
    }
    if (module != nullptr) {
      optixModuleDestroy(module);
    }
    if (stream != nullptr) {
      cudaStreamDestroy(reinterpret_cast<cudaStream_t>(stream));
    }
    if (optixContext != nullptr) {
      optixDeviceContextDestroy(optixContext);
    }
  }

  void createPipeline() {
    const std::string ptx = compilePtx();

    OptixModuleCompileOptions moduleOptions{};
    moduleOptions.optLevel = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
    moduleOptions.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_MINIMAL;

    OptixPipelineCompileOptions pipelineOptions{};
    pipelineOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
    pipelineOptions.numPayloadValues = 13;
    pipelineOptions.numAttributeValues = 2;
    pipelineOptions.pipelineLaunchParamsVariableName = "params";
    pipelineOptions.usesPrimitiveTypeFlags = OPTIX_PRIMITIVE_TYPE_FLAGS_TRIANGLE;

    std::array<char, 2048> log{};
    std::size_t logSize = log.size();
    checkOptix(optixModuleCreate(optixContext, &moduleOptions, &pipelineOptions, ptx.data(),
                                 ptx.size(), log.data(), &logSize, &module),
               "optixModuleCreate");
    if (logSize > 1) {
      spdlog::debug("OptixPathTracer module log:\n{}", log.data());
    }

    OptixProgramGroupOptions groupOptions{};
    OptixProgramGroupDesc raygenDesc{};
    raygenDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    raygenDesc.raygen.module = module;
    raygenDesc.raygen.entryFunctionName = "__raygen__rg";

    logSize = log.size();
    checkOptix(optixProgramGroupCreate(optixContext, &raygenDesc, 1, &groupOptions, log.data(),
                                       &logSize, &raygenGroup),
               "optixProgramGroupCreate(raygen)");

    OptixProgramGroupDesc missDesc{};
    missDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    missDesc.miss.module = module;
    missDesc.miss.entryFunctionName = "__miss__ms";

    logSize = log.size();
    checkOptix(optixProgramGroupCreate(optixContext, &missDesc, 1, &groupOptions, log.data(),
                                       &logSize, &missGroup),
               "optixProgramGroupCreate(miss)");

    OptixProgramGroupDesc hitDesc{};
    hitDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    hitDesc.hitgroup.moduleCH = module;
    hitDesc.hitgroup.entryFunctionNameCH = "__closesthit__ch";
    hitDesc.hitgroup.moduleAH = module;
    hitDesc.hitgroup.entryFunctionNameAH = "__anyhit__ah";

    logSize = log.size();
    checkOptix(optixProgramGroupCreate(optixContext, &hitDesc, 1, &groupOptions, log.data(),
                                       &logSize, &hitGroup),
               "optixProgramGroupCreate(hit)");

    std::array<OptixProgramGroup, 3> groups{raygenGroup, missGroup, hitGroup};
    OptixPipelineLinkOptions linkOptions{};
    linkOptions.maxTraceDepth = 1;

    logSize = log.size();
    checkOptix(optixPipelineCreate(optixContext, &pipelineOptions, &linkOptions, groups.data(),
                                   static_cast<unsigned>(groups.size()), log.data(), &logSize,
                                   &pipeline),
               "optixPipelineCreate");

    checkOptix(optixPipelineSetStackSize(pipeline, 2048, 2048, 2048, 1),
               "optixPipelineSetStackSize");
  }

  void uploadRecord(CUdeviceptr& deviceRecord, const EmptySbtRecord& record) {
    checkCuda(cudaMalloc(reinterpret_cast<void**>(&deviceRecord), sizeof(EmptySbtRecord)),
              "cudaMalloc(sbt)");
    checkCuda(cudaMemcpy(reinterpret_cast<void*>(deviceRecord), &record, sizeof(EmptySbtRecord),
                         cudaMemcpyHostToDevice),
              "cudaMemcpy(sbt)");
  }

  void createSbt() {
    EmptySbtRecord raygen{};
    EmptySbtRecord miss{};
    EmptySbtRecord hit{};
    checkOptix(optixSbtRecordPackHeader(raygenGroup, &raygen), "optixSbtRecordPackHeader(raygen)");
    checkOptix(optixSbtRecordPackHeader(missGroup, &miss), "optixSbtRecordPackHeader(miss)");
    checkOptix(optixSbtRecordPackHeader(hitGroup, &hit), "optixSbtRecordPackHeader(hit)");

    uploadRecord(raygenRecord, raygen);
    uploadRecord(missRecord, miss);
    uploadRecord(hitRecord, hit);

    sbt.raygenRecord = raygenRecord;
    sbt.missRecordBase = missRecord;
    sbt.missRecordStrideInBytes = sizeof(EmptySbtRecord);
    sbt.missRecordCount = 1;
    sbt.hitgroupRecordBase = hitRecord;
    sbt.hitgroupRecordStrideInBytes = sizeof(EmptySbtRecord);
    sbt.hitgroupRecordCount = 1;

    checkCuda(cudaMalloc(reinterpret_cast<void**>(&paramsBuffer), sizeof(LaunchParams)),
              "cudaMalloc(params)");
  }

  void freeDeviceScene() {
    for (CUdeviceptr* ptr : {&dVertices, &dUvs, &dNormals, &dIndices, &dMaterialIndices,
                             &dMaterials, &dLights, &gasBuffer}) {
      if (*ptr != 0) {
        cudaFree(reinterpret_cast<void*>(*ptr));
        *ptr = 0;
      }
    }
    gasHandle = 0;
  }

  void unregisterTextures() {
    for (auto& [id, registered] : textureResources) {
      if (registered.object != 0) {
        cudaDestroyTextureObject(registered.object);
        registered.object = 0;
      }
      if (registered.resource != nullptr) {
        cudaGraphicsUnregisterResource(registered.resource);
        registered.resource = nullptr;
      }
    }
    textureResources.clear();
    mappedResources.clear();
  }

  RegisteredTexture& registerTexture(const std::shared_ptr<Texture>& texture) {
    if (!texture) {
      throw std::runtime_error("OptixPathTracer attempted to register a null texture");
    }

    const unsigned textureId = texture->id();
    auto found = textureResources.find(textureId);
    if (found != textureResources.end()) {
      return found->second;
    }

    RegisteredTexture registered{};
    registered.texture = texture.get();
    checkCuda(cudaGraphicsGLRegisterImage(&registered.resource, textureId, GL_TEXTURE_2D,
                                          cudaGraphicsRegisterFlagsReadOnly),
              "cudaGraphicsGLRegisterImage(texture)");
    auto [it, inserted] = textureResources.emplace(textureId, registered);
    return it->second;
  }

  static cudaTextureReadMode readModeFor(TextureFormat format) {
    switch (format) {
    case TextureFormat::R16F:
    case TextureFormat::R32F:
    case TextureFormat::RG16F:
    case TextureFormat::RG32F:
    case TextureFormat::RGB16F:
    case TextureFormat::RGB32F:
    case TextureFormat::RGBA16F:
    case TextureFormat::RGBA32F:
      return cudaReadModeElementType;
    default:
      return cudaReadModeNormalizedFloat;
    }
  }

  cudaTextureObject_t mapTexture(const std::shared_ptr<Texture>& texture) {
    if (!texture) {
      return 0;
    }

    auto& registered = registerTexture(texture);
    if (registered.object != 0) {
      return registered.object;
    }

    auto* cudaStream = reinterpret_cast<cudaStream_t>(stream);
    checkCuda(cudaGraphicsMapResources(1, &registered.resource, cudaStream),
              "cudaGraphicsMapResources(texture)");
    mappedResources.push_back(registered.resource);

    cudaArray_t array = nullptr;
    checkCuda(cudaGraphicsSubResourceGetMappedArray(&array, registered.resource, 0, 0),
              "cudaGraphicsSubResourceGetMappedArray(texture)");

    cudaResourceDesc resourceDesc{};
    resourceDesc.resType = cudaResourceTypeArray;
    resourceDesc.res.array.array = array;

    cudaTextureDesc textureDesc{};
    textureDesc.addressMode[0] = texture->desc().wrapS == TextureWrap::ClampToEdge
                                     ? cudaAddressModeClamp
                                     : cudaAddressModeWrap;
    textureDesc.addressMode[1] = texture->desc().wrapT == TextureWrap::ClampToEdge
                                     ? cudaAddressModeClamp
                                     : cudaAddressModeWrap;
    textureDesc.filterMode = texture->desc().magFilter == TextureFilter::Nearest
                                 ? cudaFilterModePoint
                                 : cudaFilterModeLinear;
    textureDesc.readMode = readModeFor(texture->desc().format);
    textureDesc.normalizedCoords = 1;

    checkCuda(cudaCreateTextureObject(&registered.object, &resourceDesc, &textureDesc, nullptr),
              "cudaCreateTextureObject");
    return registered.object;
  }

  void unmapTextures() {
    for (auto& [id, registered] : textureResources) {
      if (registered.object != 0) {
        cudaDestroyTextureObject(registered.object);
        registered.object = 0;
      }
    }
    if (!mappedResources.empty()) {
      auto* cudaStream = reinterpret_cast<cudaStream_t>(stream);
      checkCuda(cudaGraphicsUnmapResources(static_cast<int>(mappedResources.size()),
                                           mappedResources.data(), cudaStream),
                "cudaGraphicsUnmapResources(textures)");
      mappedResources.clear();
    }
  }

  static MaterialData materialDataFor(const std::shared_ptr<Material>& material) {
    MaterialData data{};
    data.albedo = make_float3(0.8F, 0.8F, 0.8F);
    data.emission = make_float3(0.0F, 0.0F, 0.0F);

    if (auto pbr = std::dynamic_pointer_cast<PbrMaterial>(material)) {
      const auto& desc = pbr->desc();
      data.albedo = makeFloat3(desc.color);
      data.emission = makeFloat3(desc.emissiveColor * desc.emissiveIntensity);
      data.metallic = desc.metalness;
      data.roughness = desc.roughness;
      data.opacity = desc.opacity;
      data.alphaTest = desc.alphaTest;
      data.normalScale = desc.normalScale;
    }

    return data;
  }

  static MaterialTextures materialTexturesFor(const std::shared_ptr<Material>& material) {
    MaterialTextures textures{};
    if (auto pbr = std::dynamic_pointer_cast<PbrMaterial>(material)) {
      const auto& desc = pbr->desc();
      textures.albedoMap = desc.albedoMap;
      textures.alphaMap = desc.alphaMap;
      textures.normalMap = desc.normalMap;
      textures.metalnessMap = desc.metalnessMap;
      textures.roughnessMap = desc.roughnessMap;
      textures.aoMap = desc.aoMap;
      textures.emissiveMap = desc.emissiveMap;
    }

    return textures;
  }

  unsigned addMaterial(const std::shared_ptr<Material>& material) {
    materials.push_back(materialDataFor(material));
    materialTextures.push_back(materialTexturesFor(material));
    sceneMaterials.push_back(material);
    return static_cast<unsigned>(materials.size() - 1);
  }

  static bool sameFloat3(float3 lhs, float3 rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
  }

  static bool sameMaterialConstants(const MaterialData& lhs, const MaterialData& rhs) {
    return sameFloat3(lhs.albedo, rhs.albedo) && sameFloat3(lhs.emission, rhs.emission) &&
           lhs.metallic == rhs.metallic && lhs.roughness == rhs.roughness &&
           lhs.opacity == rhs.opacity && lhs.alphaTest == rhs.alphaTest &&
           lhs.normalScale == rhs.normalScale;
  }

  static bool sameMaterialTextures(const MaterialTextures& lhs, const MaterialTextures& rhs) {
    return lhs.albedoMap == rhs.albedoMap && lhs.alphaMap == rhs.alphaMap &&
           lhs.normalMap == rhs.normalMap && lhs.metalnessMap == rhs.metalnessMap &&
           lhs.roughnessMap == rhs.roughnessMap && lhs.aoMap == rhs.aoMap &&
           lhs.emissiveMap == rhs.emissiveMap;
  }

  bool refreshMaterialsAndLights() {
    if (sceneMaterials.size() != materials.size()) {
      return false;
    }

    bool changed = false;
    for (std::size_t idx = 0; idx < sceneMaterials.size(); ++idx) {
      const MaterialData nextMaterial = materialDataFor(sceneMaterials[idx]);
      const MaterialTextures nextTextures = materialTexturesFor(sceneMaterials[idx]);
      changed = changed || !sameMaterialConstants(materials[idx], nextMaterial) ||
                !sameMaterialTextures(materialTextures[idx], nextTextures);
      materials[idx] = nextMaterial;
      materialTextures[idx] = nextTextures;
    }

    lights.clear();
    for (std::size_t idx = 0; idx < indices.size(); ++idx) {
      addLightIfEmissive(indices[idx], materialIndices[idx]);
    }

    upload(dLights, lights.data(), lights.size() * sizeof(TriangleLight), "cudaMalloc(lights)");
    return changed;
  }

  void extractScene() {
    vertices.clear();
    uvs.clear();
    normals.clear();
    indices.clear();
    materialIndices.clear();
    materials.clear();
    materialTextures.clear();
    sceneMaterials.clear();
    lights.clear();

    if (scene == nullptr) {
      return;
    }

    scene->traverse([&](Object3D& object) {
      if (!object.visible() || object.type() != NodeType::Mesh) {
        return;
      }

      auto& mesh = static_cast<Mesh&>(object);
      auto geometry = mesh.geometry();
      if (!geometry || geometry->primitive() != PrimitiveMode::Triangles) {
        return;
      }

      const auto positions = geometry->positions();
      const auto sourceUvs = geometry->uvs();
      const auto sourceNormals = geometry->normals();
      if (positions.empty()) {
        return;
      }

      const unsigned materialIndex = addMaterial(mesh.material());
      const auto world = object.worldMatrix();
      const auto normalMatrix = glm::inverseTranspose(glm::mat3(world));
      const unsigned vertexOffset = static_cast<unsigned>(vertices.size());

      for (std::size_t idx = 0; idx + 2 < positions.size(); idx += 3) {
        const glm::vec4 worldPos =
            world * glm::vec4(positions[idx], positions[idx + 1], positions[idx + 2], 1.0F);
        vertices.push_back(make_float3(worldPos.x, worldPos.y, worldPos.z));

        glm::vec2 uv{0.0F, 0.0F};
        const std::size_t uvIndex = (idx / 3) * 2;
        if (uvIndex + 1 < sourceUvs.size()) {
          uv = {sourceUvs[uvIndex], sourceUvs[uvIndex + 1]};
        }
        uvs.push_back(make_float2(uv.x, uv.y));

        glm::vec3 normal{0.0F, 1.0F, 0.0F};
        if (idx + 2 < sourceNormals.size()) {
          normal =
              glm::normalize(normalMatrix * glm::vec3(sourceNormals[idx], sourceNormals[idx + 1],
                                                      sourceNormals[idx + 2]));
        }
        normals.push_back(makeFloat3(normal));
      }

      const auto sourceIndices = geometry->indices();
      if (!sourceIndices.empty()) {
        for (std::size_t idx = 0; idx + 2 < sourceIndices.size(); idx += 3) {
          const auto tri =
              make_uint3(vertexOffset + sourceIndices[idx], vertexOffset + sourceIndices[idx + 1],
                         vertexOffset + sourceIndices[idx + 2]);
          indices.push_back(tri);
          materialIndices.push_back(materialIndex);
          addLightIfEmissive(tri, materialIndex);
        }
      } else {
        const unsigned vertexCount = static_cast<unsigned>(positions.size() / 3);
        for (unsigned idx = 0; idx + 2 < vertexCount; idx += 3) {
          const auto tri =
              make_uint3(vertexOffset + idx, vertexOffset + idx + 1, vertexOffset + idx + 2);
          indices.push_back(tri);
          materialIndices.push_back(materialIndex);
          addLightIfEmissive(tri, materialIndex);
        }
      }
    });
  }

  void addLightIfEmissive(uint3 tri, unsigned materialIndex) {
    const auto& material = materials[materialIndex];
    const glm::vec3 emission{material.emission.x, material.emission.y, material.emission.z};
    if (glm::dot(emission, emission) <= 0.0F) {
      return;
    }

    const auto& v0 = vertices[tri.x];
    const auto& v1 = vertices[tri.y];
    const auto& v2 = vertices[tri.z];
    const glm::vec3 p0{v0.x, v0.y, v0.z};
    const glm::vec3 p1{v1.x, v1.y, v1.z};
    const glm::vec3 p2{v2.x, v2.y, v2.z};
    const glm::vec3 cross = glm::cross(p1 - p0, p2 - p0);
    const float area = 0.5F * glm::length(cross);
    if (area <= 0.0F) {
      return;
    }

    TriangleLight light{};
    light.v0 = v0;
    light.v1 = v1;
    light.v2 = v2;
    const glm::vec3 normal = glm::normalize(cross);
    light.normal = makeFloat3(normal);
    light.area = area;
    light.materialIndex = materialIndex;
    lights.push_back(light);
  }

  void uploadScene() {
    extractScene();
    freeDeviceScene();
    if (vertices.empty() || indices.empty()) {
      return;
    }

    upload(dVertices, vertices.data(), vertices.size() * sizeof(float3), "cudaMalloc(vertices)");
    upload(dUvs, uvs.data(), uvs.size() * sizeof(float2), "cudaMalloc(uvs)");
    upload(dNormals, normals.data(), normals.size() * sizeof(float3), "cudaMalloc(normals)");
    upload(dIndices, indices.data(), indices.size() * sizeof(uint3), "cudaMalloc(indices)");
    upload(dMaterialIndices, materialIndices.data(), materialIndices.size() * sizeof(unsigned),
           "cudaMalloc(materialIndices)");
    upload(dMaterials, materials.data(), materials.size() * sizeof(MaterialData),
           "cudaMalloc(materials)");
    upload(dLights, lights.data(), lights.size() * sizeof(TriangleLight), "cudaMalloc(lights)");

    CUdeviceptr vertexBuffer = dVertices;
    unsigned geometryFlags = OPTIX_GEOMETRY_FLAG_NONE;

    OptixBuildInput buildInput{};
    buildInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
    buildInput.triangleArray.vertexBuffers = &vertexBuffer;
    buildInput.triangleArray.numVertices = static_cast<unsigned>(vertices.size());
    buildInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
    buildInput.triangleArray.vertexStrideInBytes = sizeof(float3);
    buildInput.triangleArray.indexBuffer = dIndices;
    buildInput.triangleArray.numIndexTriplets = static_cast<unsigned>(indices.size());
    buildInput.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
    buildInput.triangleArray.indexStrideInBytes = sizeof(uint3);
    buildInput.triangleArray.flags = &geometryFlags;
    buildInput.triangleArray.numSbtRecords = 1;

    OptixAccelBuildOptions accelOptions{};
    accelOptions.buildFlags = OPTIX_BUILD_FLAG_ALLOW_COMPACTION;
    accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

    OptixAccelBufferSizes gasSizes{};
    checkOptix(optixAccelComputeMemoryUsage(optixContext, &accelOptions, &buildInput, 1, &gasSizes),
               "optixAccelComputeMemoryUsage");

    CUdeviceptr tempBuffer = 0;
    checkCuda(cudaMalloc(reinterpret_cast<void**>(&tempBuffer), gasSizes.tempSizeInBytes),
              "cudaMalloc(gasTemp)");
    checkCuda(cudaMalloc(reinterpret_cast<void**>(&gasBuffer), gasSizes.outputSizeInBytes),
              "cudaMalloc(gas)");

    checkOptix(optixAccelBuild(optixContext, stream, &accelOptions, &buildInput, 1, tempBuffer,
                               gasSizes.tempSizeInBytes, gasBuffer, gasSizes.outputSizeInBytes,
                               &gasHandle, nullptr, 0),
               "optixAccelBuild");
    checkCuda(cudaFree(reinterpret_cast<void*>(tempBuffer)), "cudaFree(gasTemp)");
    sceneDirty = false;
  }

  void resizeOutput(int nextWidth, int nextHeight) {
    width = nextWidth;
    height = nextHeight;
    if (outputImage != 0) {
      cudaFree(reinterpret_cast<void*>(outputImage));
      outputImage = 0;
    }
    if (accumulationImage != 0) {
      cudaFree(reinterpret_cast<void*>(accumulationImage));
      accumulationImage = 0;
    }
    outputBytes =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * kRgba16FBytesPerPixel;
    if (width > 0 && height > 0) {
      checkCuda(cudaMalloc(reinterpret_cast<void**>(&outputImage), outputBytes),
                "cudaMalloc(outputImage)");
      const std::size_t accumulationBytes =
          static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * sizeof(float4);
      checkCuda(cudaMalloc(reinterpret_cast<void**>(&accumulationImage), accumulationBytes),
                "cudaMalloc(accumulationImage)");
    }
    resetAccumulation();
  }

  void resetAccumulation() {
    frameIndex = 0;
    if (accumulationImage == 0 || width <= 0 || height <= 0) {
      return;
    }
    const std::size_t accumulationBytes =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * sizeof(float4);
    checkCuda(cudaMemset(reinterpret_cast<void*>(accumulationImage), 0, accumulationBytes),
              "cudaMemset(accumulationImage)");
  }

  void unregisterOutput() {
    if (outputResource != nullptr) {
      cudaGraphicsUnregisterResource(outputResource);
      outputResource = nullptr;
    }
    outputTextureId = 0;
  }

  void registerOutput(Texture& output) {
    if (outputTextureId == output.id()) {
      return;
    }
    if (output.desc().format != TextureFormat::RGBA16F) {
      throw std::runtime_error("OptixPathTracer currently requires an RGBA16F output texture");
    }
    unregisterOutput();
    checkCuda(cudaGraphicsGLRegisterImage(&outputResource, output.id(), GL_TEXTURE_2D,
                                          cudaGraphicsRegisterFlagsWriteDiscard),
              "cudaGraphicsGLRegisterImage(output)");
    outputTextureId = output.id();
  }

  void updateCamera(LaunchParams& params) const {
    if (camera == nullptr) {
      return;
    }

    const glm::mat4 invViewProjection =
        glm::inverse(camera->projectionMatrix() * camera->viewMatrix());
    for (int row = 0; row < 4; row++) {
      params.invViewProjection[row] =
          make_float4(invViewProjection[0][row], invViewProjection[1][row],
                      invViewProjection[2][row], invViewProjection[3][row]);
    }
    params.cameraPosition = makeFloat3(camera->worldPosition());
  }

  void updateEnvironment(LaunchParams& params) const {
    params.environmentColor = make_float3(0.02F, 0.025F, 0.035F);
    params.environmentIntensity = 1.0F;

    auto* sceneObject = dynamic_cast<Scene*>(scene);
    if (sceneObject == nullptr) {
      return;
    }

    const auto& background = sceneObject->background();
    const auto& environment = sceneObject->environment();
    if (background.type == BackgroundType::Color) {
      params.environmentColor = makeFloat3(glm::vec3(background.color));
      params.environmentIntensity = background.intensity;
      return;
    }

    // HDRI/cubemap sampling is not exported to CUDA yet, so use it as neutral miss lighting.
    params.environmentColor = make_float3(1.0F, 1.0F, 1.0F);
    params.environmentIntensity = background.intensity * environment.intensity;
  }

  void prepareMaterialTextures() {
    if (materials.empty()) {
      return;
    }

    for (std::size_t idx = 0; idx < materials.size(); ++idx) {
      const auto& textures = materialTextures[idx];
      auto& material = materials[idx];
      material.albedoMap = mapTexture(textures.albedoMap);
      material.alphaMap = mapTexture(textures.alphaMap);
      material.normalMap = mapTexture(textures.normalMap);
      material.metalnessMap = mapTexture(textures.metalnessMap);
      material.roughnessMap = mapTexture(textures.roughnessMap);
      material.aoMap = mapTexture(textures.aoMap);
      material.emissiveMap = mapTexture(textures.emissiveMap);
      material.hasAlbedoMap = material.albedoMap != 0 ? 1U : 0U;
      material.hasAlphaMap = material.alphaMap != 0 ? 1U : 0U;
      material.hasNormalMap = material.normalMap != 0 ? 1U : 0U;
      material.hasMetalnessMap = material.metalnessMap != 0 ? 1U : 0U;
      material.hasRoughnessMap = material.roughnessMap != 0 ? 1U : 0U;
      material.hasAoMap = material.aoMap != 0 ? 1U : 0U;
      material.hasEmissiveMap = material.emissiveMap != 0 ? 1U : 0U;
    }

    checkCuda(cudaMemcpy(reinterpret_cast<void*>(dMaterials), materials.data(),
                         materials.size() * sizeof(MaterialData), cudaMemcpyHostToDevice),
              "cudaMemcpy(materials)");
  }

  void prepareEnvironmentTexture(LaunchParams& params) {
    auto* sceneObject = dynamic_cast<Scene*>(scene);
    if (sceneObject == nullptr) {
      return;
    }

    const auto& background = sceneObject->background();
    const auto& environment = sceneObject->environment();
    std::shared_ptr<Texture> texture =
        background.texture ? background.texture : environment.equirect;
    if (!texture) {
      return;
    }

    params.environmentMap = mapTexture(texture);
    params.hasEnvironmentMap = params.environmentMap != 0 ? 1U : 0U;
  }

  void renderTo(Texture& output) {
    if (width <= 0 || height <= 0 || outputImage == 0) {
      return;
    }
    if (sceneDirty) {
      uploadScene();
    }
    if (gasHandle == 0) {
      return;
    }

    registerOutput(output);
    if (refreshMaterialsAndLights()) {
      resetAccumulation();
    }

    try {
      prepareMaterialTextures();

      LaunchParams params{};
      params.output = outputImage;
      params.accumulation = accumulationImage;
      params.width = static_cast<unsigned>(width);
      params.height = static_cast<unsigned>(height);
      params.samplesPerPixel = static_cast<unsigned>(std::max(1, desc.samplesPerPixel));
      params.maxBounces = static_cast<unsigned>(std::max(1, desc.maxBounces));
      params.frameIndex = accumulate ? frameIndex++ : 0;
      params.accumulate = accumulate ? 1U : 0U;
      params.integratorMode = static_cast<unsigned>(desc.integratorMode);
      params.samplingMode = static_cast<unsigned>(desc.samplingMode);
      params.environmentMode = static_cast<unsigned>(desc.environmentMode);
      params.materialMode = static_cast<unsigned>(desc.materialMode);
      params.misMode = static_cast<unsigned>(desc.misMode);
      params.debugView = static_cast<unsigned>(desc.debugView);
      params.enableTextures = desc.enableTextures ? 1U : 0U;
      params.enableNormalMaps = desc.enableNormalMaps ? 1U : 0U;
      params.enableAlpha = desc.enableAlpha ? 1U : 0U;
      params.enableMirrorReflection = desc.enableMirrorReflection ? 1U : 0U;
      params.enableDirectLighting = desc.enableDirectLighting ? 1U : 0U;
      params.enableShadowRays = desc.enableShadowRays ? 1U : 0U;
      params.enableEmissiveLights = desc.enableEmissiveLights ? 1U : 0U;
      params.enableRussianRoulette = desc.enableRussianRoulette ? 1U : 0U;
      params.traversable = gasHandle;
      params.vertices = dVertices;
      params.uvs = dUvs;
      params.normals = dNormals;
      params.indices = dIndices;
      params.materialIndices = dMaterialIndices;
      params.materials = dMaterials;
      params.lights = dLights;
      params.lightCount = static_cast<unsigned>(lights.size());
      updateCamera(params);
      updateEnvironment(params);
      prepareEnvironmentTexture(params);

      checkCuda(cudaMemcpy(reinterpret_cast<void*>(paramsBuffer), &params, sizeof(LaunchParams),
                           cudaMemcpyHostToDevice),
                "cudaMemcpy(params)");
      checkOptix(optixLaunch(pipeline, stream, paramsBuffer, sizeof(LaunchParams), &sbt,
                             static_cast<unsigned>(width), static_cast<unsigned>(height), 1),
                 "optixLaunch");

      cudaGraphicsResource* resources[] = {outputResource};
      auto* cudaStream = reinterpret_cast<cudaStream_t>(stream);
      checkCuda(cudaGraphicsMapResources(1, resources, cudaStream), "cudaGraphicsMapResources");
      cudaArray_t outputArray = nullptr;
      checkCuda(cudaGraphicsSubResourceGetMappedArray(&outputArray, outputResource, 0, 0),
                "cudaGraphicsSubResourceGetMappedArray(output)");
      const std::size_t rowBytes = static_cast<std::size_t>(width) * kRgba16FBytesPerPixel;
      checkCuda(cudaMemcpy2DToArrayAsync(outputArray, 0, 0, reinterpret_cast<void*>(outputImage),
                                         rowBytes, rowBytes, static_cast<std::size_t>(height),
                                         cudaMemcpyDeviceToDevice, cudaStream),
                "cudaMemcpy2DToArrayAsync(output)");
      checkCuda(cudaGraphicsUnmapResources(1, resources, cudaStream), "cudaGraphicsUnmapResources");
      unmapTextures();
    } catch (...) {
      unmapTextures();
      throw;
    }
  }
};

OptixPathTracer::OptixPathTracer(const OptixPathTracerDesc& desc)
    : impl_(new Impl(desc)) {
}

OptixPathTracer::~OptixPathTracer() {
  delete impl_;
}

void OptixPathTracer::setScene(Object3D* scene) {
  impl_->scene = scene;
  impl_->sceneDirty = true;
  impl_->resetAccumulation();
}

void OptixPathTracer::setCamera(Camera* camera) {
  impl_->camera = camera;
  impl_->resetAccumulation();
}

void OptixPathTracer::setSize(int width, int height) {
  impl_->resizeOutput(width, height);
}

void OptixPathTracer::setSamplesPerPixel(int samplesPerPixel) {
  impl_->desc.samplesPerPixel = samplesPerPixel;
  impl_->resetAccumulation();
}

void OptixPathTracer::setMaxBounces(int maxBounces) {
  impl_->desc.maxBounces = maxBounces;
  impl_->resetAccumulation();
}

void OptixPathTracer::setDesc(const OptixPathTracerDesc& desc) {
  impl_->desc = desc;
  impl_->resetAccumulation();
}

void OptixPathTracer::setAccumulate(bool accumulate) {
  impl_->accumulate = accumulate;
  impl_->resetAccumulation();
}

void OptixPathTracer::resetAccumulation() {
  impl_->resetAccumulation();
}

void OptixPathTracer::renderTo(Texture& output) {
  impl_->renderTo(output);
}

} // namespace blkhurst

#endif // BLKHURST_ENABLE_OPTIX
