#include <blkhurst/assets/asset_loader.hpp>
#include <blkhurst/model/model_processor.hpp>
#include <blkhurst/objects/mesh.hpp>
#include <blkhurst/util/assets.hpp>

#include <assimp/GltfMaterial.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <assimp/types.h>
#include <spdlog/spdlog.h>
#include <string>

namespace blkhurst {

ModelCPU ModelProcessor::load(const std::string& path, const ModelProcessorDesc& desc,
                              AssetLoader* loader) {
  // Resolve Model Path
  auto resolvedPath = assets::find(path);
  if (!resolvedPath) {
    spdlog::warn("ModelProcessor asset not found '{}'", path);
    return {.success = false};
  }

  // Create Context
  ModelProcessorContext context;
  context.desc = desc;
  context.modelPath = *resolvedPath;
  context.assetLoader = loader;

  // Import Scene
  Assimp::Importer importer;
  const unsigned int flags = composeFlags(desc);
  const aiScene* scene = importer.ReadFile(context.modelPath, flags);

  // Validate
  bool sceneNull = (scene == nullptr);
  bool sceneIncomplete =
      ((scene != nullptr) && ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0U));
  bool rootNodeNull = ((scene != nullptr) && (scene->mRootNode == nullptr));

  if (sceneNull || sceneIncomplete || rootNodeNull) {
    spdlog::error("ModelProcessor failed to load '{}' : {}", context.modelPath,
                  importer.GetErrorString());
    return {.success = false};
  }

  // Convert AssImp to Object3D
  ModelCPU modelCPU;
  modelCPU.root = processNode(scene, scene->mRootNode, context);
  modelCPU.sourcePath = context.modelPath;
  modelCPU.success = true;
  modelCPU.meshCount = scene->mNumMeshes;
  modelCPU.materialCount = scene->mNumMaterials;
  modelCPU.textureCount = scene->mNumTextures;

  spdlog::debug("ModelProcessor loaded '{}' (meshes={}, materials={}, textures={})",
                context.modelPath, scene->mNumMeshes, scene->mNumMaterials, scene->mNumTextures);

  return modelCPU;
}

//
NodeCPU ModelProcessor::processNode(const aiScene* scene, const aiNode* node,
                                    ModelProcessorContext& context) {
  // Create NodeCPU For This Node
  NodeCPU group;
  if (node->mName.length > 0) {
    group.name = node->mName.C_Str();
  }

  // Transform
  struct TRS {
    glm::vec3 t;
    glm::quat r;
    glm::vec3 s;
  };
  auto decompose = [](const aiMatrix4x4& mat) {
    aiVector3D scale;
    aiQuaternion rotation;
    aiVector3D translation;
    mat.Decompose(scale, rotation, translation);
    return TRS{{translation.x, translation.y, translation.z},
               {rotation.w, rotation.x, rotation.y, rotation.z},
               {scale.x, scale.y, scale.z}};
  };
  const TRS trs = decompose(node->mTransformation);
  group.position = trs.t;
  group.rotation = trs.r;
  group.scale = trs.s;

  // Process Node
  const std::span<aiMesh* const> sceneMeshes{scene->mMeshes, scene->mNumMeshes};
  const std::span<aiMaterial* const> sceneMaterials{scene->mMaterials, scene->mNumMaterials};
  const std::span<const unsigned int> meshIndices{node->mMeshes, node->mNumMeshes};
  //
  for (auto meshIndex : meshIndices) {
    aiMesh* sceneMesh = sceneMeshes[meshIndex];
    // Geometry
    auto geometry = processGeometry(*sceneMesh);
    // Material
    const aiMaterial* material = sceneMaterials[sceneMesh->mMaterialIndex];
    auto pbrMaterial = buildMaterial(scene, sceneMesh, material, context);
    // Mesh
    MeshCPU meshCPU;
    meshCPU.name = sceneMesh->mName.C_Str();
    meshCPU.geometry = geometry;
    meshCPU.material = pbrMaterial;
    group.meshes.push_back(std::move(meshCPU));
  }

  // Process Nodes Children Recursively
  const std::span<aiNode* const> children{node->mChildren, node->mNumChildren};
  //
  for (auto* childNode : children) {
    auto childObj = processNode(scene, childNode, context);
    group.children.push_back(childObj);
  }

  return group;
}

GeometryCPU ModelProcessor::processGeometry(const aiMesh& mesh) {
  // Create GeometryCPU
  GeometryCPU geometry;

  // Position
  const std::span<aiVector3D> verts{mesh.mVertices, mesh.mNumVertices};
  for (const auto& vertex : verts) {
    geometry.positions.insert(geometry.positions.end(), {vertex.x, vertex.y, vertex.z});
  }

  // Indices
  if (mesh.HasFaces()) {
    const std::span<aiFace> faces{mesh.mFaces, mesh.mNumFaces};
    for (const auto& face : faces) {
      if (face.mNumIndices == 3) { // aiProcess_Triangulate should ensure this
        const std::span<const unsigned int> faceIndices{face.mIndices, face.mNumIndices};
        geometry.indices.push_back(faceIndices[0]);
        geometry.indices.push_back(faceIndices[1]);
        geometry.indices.push_back(faceIndices[2]);
      }
    }
  }

  // Normal
  if (mesh.HasNormals()) {
    const std::span<aiVector3D> norms{mesh.mNormals, mesh.mNumVertices};
    for (const auto& normal : norms) {
      geometry.normals.insert(geometry.normals.end(), {normal.x, normal.y, normal.z});
    }
  }

  // UV (Channel 0)
  if (mesh.HasTextureCoords(0)) {
    const std::span<aiVector3D> textureCoords{mesh.mTextureCoords[0], mesh.mNumVertices};
    for (const auto& textureCoord : textureCoords) {
      geometry.uvs.push_back(textureCoord.x);
      geometry.uvs.push_back(textureCoord.y);
    }
  }

  // Vertex Colors (Channel 0)
  if (mesh.HasVertexColors(0)) {
    const std::span<aiColor4D> colors{mesh.mColors[0], mesh.mNumVertices};
    for (const auto& color : colors) {
      geometry.colors.insert(geometry.colors.end(), {color.r, color.g, color.b});
    }
  }

  // Tangent
  // if (mesh.HasTangentsAndBitangents()) {
  //   const std::span<aiVector3D> tangs{mesh.mTangents, mesh.mNumVertices};
  //   const std::span<aiVector3D> bitangs{mesh.mBitangents, mesh.mNumVertices};
  //   for (const auto& tangent : tangs) {
  //     geometry.tangents.insert(geometry.tangents.end(), {tangent.x, tangent.y, tangent.z});
  //   }
  //   for (const auto& bitangent : bitangs) {
  //     geometry.bitangents.insert(geometry.bitangents.end(),
  //                                {bitangent.x, bitangent.y, bitangent.z});
  //   }
  // }

  // UV (Channel 1)
  // if (mesh.HasTextureCoords(1)) {
  //   const std::span<aiVector3D> textureCoords{mesh.mTextureCoords[1], mesh.mNumVertices};
  //   for (const auto& textureCoord : textureCoords) {
  //     geometry.uvs1.push_back(textureCoord.x);
  //     geometry.uvs1.push_back(textureCoord.y);
  //   }
  // }

  return geometry;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::shared_ptr<PbrMaterial> ModelProcessor::buildMaterial(const aiScene* scene,
                                                           const aiMesh* sceneMesh,
                                                           const aiMaterial* material,
                                                           ModelProcessorContext& context) {
  // Create PBRMaterial
  auto pbrMaterial = PbrMaterial::create({});
  pbrMaterial->setName(material->GetName().C_Str());

  // Double Sided
  int twoSided = 0;
  if (material->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS) {
    pbrMaterial->setCullFace(twoSided != 0 ? CullFace::None : CullFace::Back);
  }

  // Opacity
  float opacity = 1.0F;
  material->Get(AI_MATKEY_OPACITY, opacity);
  pbrMaterial->setOpacity(opacity);
  //
  aiString alphaTexture;
  if (material->GetTexture(aiTextureType_OPACITY, 0, &alphaTexture) == AI_SUCCESS) {
    pbrMaterial->setAlphaMap(loadTexture(scene, alphaTexture, {.srgb = false}, context));
    spdlog::trace("ModelProcessor Alpha Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // AlphaMode (Opaque, Mask, Blend)
  aiString alphaMode;
  const std::string mode = material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS
                               ? alphaMode.C_Str()
                               : "OPAQUE";
  if (mode == "BLEND") {
    pbrMaterial->setBlend(true);
    pbrMaterial->setAlphaTest(-1.0F);
    pbrMaterial->setDepthWrite(false);
  } else if (mode == "MASK") {
    float cutoff = 0.5F;
    material->Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff);
    pbrMaterial->setBlend(false);
    pbrMaterial->setAlphaTest(cutoff);
    pbrMaterial->setDepthWrite(true);
  } else { // OPAQUE
    pbrMaterial->setBlend(false);
    pbrMaterial->setAlphaTest(-1.0F);
    pbrMaterial->setDepthWrite(true);
  }

  // Base Color
  aiColor4D base{};
  aiColor3D diffuse(1, 1, 1);
  bool hasBase = material->Get(AI_MATKEY_BASE_COLOR, base) == AI_SUCCESS;
  bool hasDiffuse = material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS;
  if (hasBase) {
    // Multiply BaseAlpha with Opacity // TODO: Set Blend?
    pbrMaterial->setColor({base.r, base.g, base.b});
    pbrMaterial->setOpacity(base.a * pbrMaterial->desc().opacity);
  } else if (hasDiffuse) {
    pbrMaterial->setColor({diffuse.r, diffuse.g, diffuse.b});
  }
  //
  aiString albedoTexture;
  if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &albedoTexture) == AI_SUCCESS ||
      material->GetTexture(aiTextureType_DIFFUSE, 0, &albedoTexture) == AI_SUCCESS) {
    pbrMaterial->setAlbedoMap(loadTexture(scene, albedoTexture, {.srgb = true}, context));
    spdlog::trace("ModelProcessor: Albedo Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Normal
  aiString normalTexture;
  if (material->GetTexture(aiTextureType_NORMALS, 0, &normalTexture) == AI_SUCCESS ||
      material->GetTexture(aiTextureType_HEIGHT, 0, &normalTexture) == AI_SUCCESS) {
    pbrMaterial->setNormalMap(loadTexture(scene, normalTexture, {.srgb = false}, context));
    spdlog::trace("ModelProcessor: Normal Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Metalness (ORM points to same texture)
  float metal = 0.0F;
  material->Get(AI_MATKEY_METALLIC_FACTOR, metal);
  pbrMaterial->setMetalness(metal);
  //
  aiString metalnessTexture;
  if (material->GetTexture(aiTextureType_METALNESS, 0, &metalnessTexture) == AI_SUCCESS) {
    pbrMaterial->setMetalnessMap(loadTexture(scene, metalnessTexture, {.srgb = false}, context));
    spdlog::trace("ModelProcessor: Metalness Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Roughness
  float rough = 1.0F;
  material->Get(AI_MATKEY_ROUGHNESS_FACTOR, rough);
  pbrMaterial->setRoughness(rough);
  //
  aiString roughnessTexture;
  if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &roughnessTexture) == AI_SUCCESS) {
    pbrMaterial->setRoughnessMap(loadTexture(scene, roughnessTexture, {.srgb = false}, context));
    spdlog::trace("ModelProcessor: Roughness Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Ao
  aiString aoTexture;
  if (material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &aoTexture) == AI_SUCCESS ||
      material->GetTexture(aiTextureType_LIGHTMAP, 0, &aoTexture) == AI_SUCCESS) {
    pbrMaterial->setAoMap(loadTexture(scene, aoTexture, {.srgb = false}, context));
    pbrMaterial->setAoIntensity(1.0F);
    spdlog::trace("ModelProcessor: AO Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Emissive
  aiColor3D emissive(0, 0, 0);
  if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
    pbrMaterial->setEmissiveColor({emissive.r, emissive.g, emissive.b});
    // spdlog::trace("ModelProcessor: Emissive Color Set for '{}'", pbrMaterial->uuidString());
  }
  //
  float emissiveIntensity = 1.0F;
  if (material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity) == AI_SUCCESS) {
    pbrMaterial->setEmissiveIntensity(emissiveIntensity);
    spdlog::trace("ModelProcessor: Emissive Intensity Set for '{}'", pbrMaterial->uuidString());
  }
  //
  aiString emissiveTexture;
  if (material->GetTexture(aiTextureType_EMISSIVE, 0, &emissiveTexture) == AI_SUCCESS) {
    pbrMaterial->setEmissiveMap(loadTexture(scene, emissiveTexture, {.srgb = true}, context));
    spdlog::trace("ModelProcessor: Emissive Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Flat Shading
  int shading = 0;
  const bool hasShading = material->Get(AI_MATKEY_SHADING_MODEL, shading) == AI_SUCCESS;
  if (hasShading && shading == aiShadingMode_Flat) {
    pbrMaterial->setFlatShading(true);
    spdlog::trace("ModelProcessor: Material '{}' has Flat Shading", pbrMaterial->uuidString());
  }

  // VertexColors
  if (sceneMesh->HasVertexColors(0)) {
    pbrMaterial->setVertexColors(true);
    spdlog::trace("ModelProcessor: Mesh '{}' has Vertex Colors", sceneMesh->mName.C_Str());
  }

  // UvTransform
  aiUVTransform uvT{};
  if (material->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_BASE_COLOR, 0), uvT) == AI_SUCCESS ||
      material->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE, 0), uvT) == AI_SUCCESS) {
    pbrMaterial->setUvRepeat({uvT.mScaling.x, uvT.mScaling.y});
    pbrMaterial->setUvOffset({uvT.mTranslation.x, uvT.mTranslation.y});
    pbrMaterial->setUvRotation(uvT.mRotation); // radians
  }

  // TODO: Fallback approximation for Metalness/Roughness using Phong Shininess

  return pbrMaterial;
}

// --------------- Helpers ---------------

unsigned int ModelProcessor::composeFlags(const ModelProcessorDesc& desc) {
  unsigned int flags = 0U;
  if (desc.triangulate) {
    flags |= aiProcess_Triangulate;
  }
  if (desc.genSmoothNormals) {
    flags |= aiProcess_GenSmoothNormals;
  } else {
    flags |= aiProcess_GenNormals;
  }
  if (desc.calcTangents) {
    flags |= aiProcess_CalcTangentSpace;
  }
  flags |= aiProcess_ImproveCacheLocality | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType;
  if (desc.optimize) {
    flags |= aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph;
  }
  return flags;
}

std::shared_ptr<Texture> ModelProcessor::loadTexture(const aiScene* scene,
                                                     const aiString& texturePath,
                                                     const TextureLoaderDesc& desc,
                                                     ModelProcessorContext& context) {
  if (texturePath.length == 0) {
    spdlog::warn("ModelProcessor: empty texture path");
    return nullptr;
  }

  const std::string pathStr = texturePath.C_Str();

  // Check if already loaded
  auto foundTexture = context.textureCache.find(pathStr);
  if (foundTexture != context.textureCache.end()) {
    spdlog::trace("ModelProcessor textureCache hit: {}", pathStr);
    return foundTexture->second;
  }

  // 1) External files - relative to model, then global
  if (!pathStr.empty() && pathStr.front() != '*') {
    const auto modelDir = std::filesystem::path(context.modelPath).parent_path();
    const auto candidate = (modelDir / pathStr).generic_string();

    if (auto rel = assets::find(candidate)) {
      auto texture = loadTextureFile(*rel, desc, context);
      context.textureCache[pathStr] = texture;
      return texture;
    }
    if (auto direct = assets::find(pathStr)) {
      auto texture = loadTextureFile(*direct, desc, context);
      context.textureCache[pathStr] = texture;
      return texture;
    }
  }

  // 2) Embedded - "*<index>"
  if (!pathStr.empty() && pathStr.front() == '*') {
    // Resolve Index
    int index = 0;
    try {
      index = std::stoi(pathStr.substr(1));
    } catch (...) {
      spdlog::error("ModelProcessor: invalid embedded texture index '{}'", pathStr);
      return nullptr;
    }

    if (index < 0 || static_cast<unsigned>(index) >= scene->mNumTextures) {
      spdlog::error("ModelProcessor: embedded texture index out of range ({})", index);
      return nullptr;
    }

    const std::span<aiTexture* const> sceneTextures{scene->mTextures, scene->mNumTextures};
    const aiTexture* sceneTexture = sceneTextures[index];
    if (sceneTexture == nullptr) {
      spdlog::error("ModelProcessor: embedded texture {} is null", index);
      return nullptr;
    }

    // If mHeight is 0 - Compressed (Encoded Blob PNG/JPG/HDR/etc.)
    if (sceneTexture->mHeight == 0) {
      const auto* raw = std::bit_cast<const unsigned char*>(sceneTexture->pcData);
      const size_t size = sceneTexture->mWidth; // Assimp stores byte count in mWidth.
      auto blob = std::vector<unsigned char>(size);
      std::memcpy(blob.data(), raw, size);
      auto texture = loadTextureEmbedded(std::move(blob), desc, context);
      context.textureCache[pathStr] = texture;
      return texture;
    }

    // if mHeight not 0 - Uncompressed RGBA8 array of aiTexel
    const int width = static_cast<int>(sceneTexture->mWidth);
    const int height = static_cast<int>(sceneTexture->mHeight);
    const size_t texelCount = static_cast<size_t>(width) * static_cast<size_t>(height);

    const std::span<const aiTexel> texels{sceneTexture->pcData, texelCount};
    // loadModel runs in a thread, so already asynchronous
    std::vector<unsigned char> rgba;
    rgba.reserve(texelCount * 4);
    for (const aiTexel& texel : texels) {
      rgba.push_back(texel.r);
      rgba.push_back(texel.g);
      rgba.push_back(texel.b);
      rgba.push_back(texel.a);
    }
    auto texture = loadTextureRgba8(width, height, std::move(rgba), desc, context);
    context.textureCache[pathStr] = texture;
    return texture;
  }

  spdlog::warn("ModelProcessor: texture not found '{}'", pathStr);
  return nullptr;
}

// Use AssetLoader if available (async); else load synchronously
std::shared_ptr<Texture> ModelProcessor::loadTextureFile(const std::string& path,
                                                         const TextureLoaderDesc& desc,
                                                         ModelProcessorContext& context) {

  if (context.assetLoader != nullptr) {
    return context.assetLoader->loadTexture(path, desc);
  }
  return TextureLoader::load(path, desc);
}

std::shared_ptr<Texture> ModelProcessor::loadTextureEmbedded(std::vector<uint8_t> data,
                                                             const TextureLoaderDesc& desc,
                                                             ModelProcessorContext& context) {
  //* Imperative we move data to avoid dangling pointer
  if (context.assetLoader != nullptr) {
    return context.assetLoader->loadTextureFromMemory(std::move(data), desc);
  }
  return TextureLoader::loadFromMemory(data.data(), data.size(), desc);
}

std::shared_ptr<Texture> ModelProcessor::loadTextureRgba8(int width, int height,
                                                          std::vector<uint8_t> rgba,
                                                          const TextureLoaderDesc& desc,
                                                          ModelProcessorContext& context) {
  //* Imperative we move rgba to avoid dangling pointer
  if (context.assetLoader != nullptr) {
    return context.assetLoader->loadTextureFromRgba8(width, height, std::move(rgba), desc);
  }
  return TextureLoader::loadFromRgba8(width, height, rgba.data(), desc);
}

} // namespace blkhurst
