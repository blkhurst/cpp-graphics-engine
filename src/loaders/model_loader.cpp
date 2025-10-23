#include <blkhurst/loaders/model_loader.hpp>
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

std::unique_ptr<Group> ModelLoader::load(const std::string& path, const ModelLoaderDesc& desc) {
  // Resolve Model Path
  auto resolvedPath = assets::find(path);
  if (!resolvedPath) {
    spdlog::error("ModelLoader asset not found '{}'", path);
    return makeFallbackModel();
  }

  // Create Context
  ModelLoaderContext context;
  context.desc = desc;
  context.modelPath = *resolvedPath;

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
    spdlog::error("ModelLoader failed to load '{}' : {}", context.modelPath,
                  importer.GetErrorString());
    return makeFallbackModel();
  }

  // Convert AssImp to Object3D
  auto model = processNode(scene, scene->mRootNode, context);

  spdlog::debug("ModelLoader loaded '{}' (meshes={}, materials={}, textures={})", context.modelPath,
                scene->mNumMeshes, scene->mNumMaterials, scene->mNumTextures);

  return model;
}

//
std::unique_ptr<Group> ModelLoader::processNode(const aiScene* scene, const aiNode* node,
                                                ModelLoaderContext& context) {
  // Create Group For This Node
  auto group = std::make_unique<Group>();
  if (node->mName.length > 0) {
    group->setName(node->mName.C_Str());
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
  group->setPosition(trs.t);
  group->setRotation(trs.r);
  group->setScale(trs.s);

  // Process Node
  const std::span<aiMesh* const> sceneMeshes{scene->mMeshes, scene->mNumMeshes};
  const std::span<aiMaterial* const> sceneMaterials{scene->mMaterials, scene->mNumMaterials};
  const std::span<const unsigned int> meshIndices{node->mMeshes, node->mNumMeshes};
  //
  for (auto meshIndex : meshIndices) {
    aiMesh* sceneMesh = sceneMeshes[meshIndex];
    // Geometry
    auto geometry = buildGeometry(*sceneMesh);
    // Material
    const aiMaterial* material = sceneMaterials[sceneMesh->mMaterialIndex];
    auto pbrMaterial = buildMaterial(scene, sceneMesh, material, context);
    // Mesh
    auto meshObj = Mesh::create(geometry, pbrMaterial);
    meshObj->setName(sceneMesh->mName.C_Str());
    group->addChild(std::move(meshObj));
  }

  // Process Nodes Children Recursively
  const std::span<aiNode* const> children{node->mChildren, node->mNumChildren};
  //
  for (auto* childNode : children) {
    auto childObj = processNode(scene, childNode, context);
    group->addChild(std::move(childObj));
  }

  return group;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::shared_ptr<Geometry> ModelLoader::buildGeometry(const aiMesh& mesh) {
  // Create Geometry
  auto geometry = Geometry::create();

  // Position
  const std::span<aiVector3D> verts{mesh.mVertices, mesh.mNumVertices};
  std::vector<float> positions;
  for (const auto& vertex : verts) {
    positions.insert(positions.end(), {vertex.x, vertex.y, vertex.z});
  }
  geometry->setAttribute(Attrib::Position, positions, 3);

  // Indices
  if (mesh.HasFaces()) {
    const std::span<aiFace> faces{mesh.mFaces, mesh.mNumFaces};
    std::vector<unsigned> indices;
    for (const auto& face : faces) {
      if (face.mNumIndices == 3) { // aiProcess_Triangulate should ensure this
        const std::span<const unsigned int> faceIndices{face.mIndices, face.mNumIndices};
        indices.push_back(faceIndices[0]);
        indices.push_back(faceIndices[1]);
        indices.push_back(faceIndices[2]);
      }
    }
    if (!indices.empty()) {
      geometry->setIndex(indices);
    }
  }

  // Normal
  if (mesh.HasNormals()) {
    const std::span<aiVector3D> norms{mesh.mNormals, mesh.mNumVertices};
    std::vector<float> normals;
    for (const auto& normal : norms) {
      normals.insert(normals.end(), {normal.x, normal.y, normal.z});
    }
    geometry->setAttribute(Attrib::Normal, normals, 3);
  }

  // UV (Channel 0)
  if (mesh.HasTextureCoords(0)) {
    const std::span<aiVector3D> textureCoords{mesh.mTextureCoords[0], mesh.mNumVertices};
    std::vector<float> uvs;
    for (const auto& textureCoord : textureCoords) {
      uvs.push_back(textureCoord.x);
      uvs.push_back(textureCoord.y);
    }
    geometry->setAttribute(Attrib::Uv, uvs, 2);
  }

  // Vertex Colors (Channel 0)
  if (mesh.HasVertexColors(0)) {
    const std::span<aiColor4D> colors{mesh.mColors[0], mesh.mNumVertices};
    std::vector<float> cols;
    for (const auto& color : colors) {
      cols.insert(cols.end(), {color.r, color.g, color.b});
    }
    geometry->setAttribute(Attrib::Color, cols, 3);
  }

  // Tangent
  // if (mesh.HasTangentsAndBitangents()) {
  //   const std::span<aiVector3D> tangs{mesh.mTangents, mesh.mNumVertices};
  //   const std::span<aiVector3D> bitangs{mesh.mBitangents, mesh.mNumVertices};
  //   std::vector<float> tangents;
  //   std::vector<float> bitangents;
  //   for (const auto& tangent : tangs) {
  //     tangents.insert(tangents.end(), {tangent.x, tangent.y, tangent.z});
  //   }
  //   for (const auto& bitangent : bitangs) {
  //     bitangents.insert(bitangents.end(), {bitangent.x, bitangent.y, bitangent.z});
  //   }
  //   geometry->setAttribute(Attrib::Tangent, tangents, 3);
  //   geometry->setAttribute(Attrib::Bitangent, bitangents, 3);
  //   // Bitangent can be reconstructed in shader
  // }

  // UV (Channel 1)
  // if (mesh.HasTextureCoords(1)) {
  //   const std::span<aiVector3D> textureCoords{mesh.mTextureCoords[1], mesh.mNumVertices};
  //   std::vector<float> uvs;
  //   for (const auto& textureCoord : textureCoords) {
  //     uvs.push_back(textureCoord.x);
  //     uvs.push_back(textureCoord.y);
  //   }
  //   geometry->setAttribute(Attrib::Uv1, uvs, 2);
  // }

  return geometry;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::shared_ptr<PbrMaterial> ModelLoader::buildMaterial(const aiScene* scene,
                                                        const aiMesh* sceneMesh,
                                                        const aiMaterial* material,
                                                        ModelLoaderContext& context) {
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
    spdlog::trace("ModelLoader Alpha Map Loaded for '{}'", pbrMaterial->uuidString());
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
    spdlog::trace("ModelLoader: Albedo Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Normal
  aiString normalTexture;
  if (material->GetTexture(aiTextureType_NORMALS, 0, &normalTexture) == AI_SUCCESS ||
      material->GetTexture(aiTextureType_HEIGHT, 0, &normalTexture) == AI_SUCCESS) {
    pbrMaterial->setNormalMap(loadTexture(scene, normalTexture, {.srgb = false}, context));
    spdlog::trace("ModelLoader: Normal Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Metalness (ORM points to same texture)
  float metal = 0.0F;
  material->Get(AI_MATKEY_METALLIC_FACTOR, metal);
  pbrMaterial->setMetalness(metal);
  //
  aiString metalnessTexture;
  if (material->GetTexture(aiTextureType_METALNESS, 0, &metalnessTexture) == AI_SUCCESS) {
    pbrMaterial->setMetalnessMap(loadTexture(scene, metalnessTexture, {.srgb = false}, context));
    spdlog::trace("ModelLoader: Metalness Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Roughness
  float rough = 1.0F;
  material->Get(AI_MATKEY_ROUGHNESS_FACTOR, rough);
  pbrMaterial->setRoughness(rough);
  //
  aiString roughnessTexture;
  if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &roughnessTexture) == AI_SUCCESS) {
    pbrMaterial->setRoughnessMap(loadTexture(scene, roughnessTexture, {.srgb = false}, context));
    spdlog::trace("ModelLoader: Roughness Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Ao
  aiString aoTexture;
  if (material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &aoTexture) == AI_SUCCESS ||
      material->GetTexture(aiTextureType_LIGHTMAP, 0, &aoTexture) == AI_SUCCESS) {
    pbrMaterial->setAoMap(loadTexture(scene, aoTexture, {.srgb = false}, context));
    pbrMaterial->setAoIntensity(1.0F);
    spdlog::trace("ModelLoader: AO Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Emissive
  aiColor3D emissive(0, 0, 0);
  if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
    pbrMaterial->setEmissiveColor({emissive.r, emissive.g, emissive.b});
    // spdlog::trace("ModelLoader: Emissive Color Set for '{}'", pbrMaterial->uuidString());
  }
  //
  float emissiveIntensity = 1.0F;
  if (material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity) == AI_SUCCESS) {
    pbrMaterial->setEmissiveIntensity(emissiveIntensity);
    spdlog::trace("ModelLoader: Emissive Intensity Set for '{}'", pbrMaterial->uuidString());
  }
  //
  aiString emissiveTexture;
  if (material->GetTexture(aiTextureType_EMISSIVE, 0, &emissiveTexture) == AI_SUCCESS) {
    pbrMaterial->setEmissiveMap(loadTexture(scene, emissiveTexture, {.srgb = true}, context));
    spdlog::trace("ModelLoader: Emissive Map Loaded for '{}'", pbrMaterial->uuidString());
  }

  // Flat Shading
  int shading = 0;
  const bool hasShading = material->Get(AI_MATKEY_SHADING_MODEL, shading) == AI_SUCCESS;
  if (hasShading && shading == aiShadingMode_Flat) {
    pbrMaterial->setFlatShading(true);
    spdlog::trace("ModelLoader: Material '{}' has Flat Shading", pbrMaterial->uuidString());
  }

  // VertexColors
  if (sceneMesh->HasVertexColors(0)) {
    pbrMaterial->setVertexColors(true);
    spdlog::trace("ModelLoader: Mesh '{}' has Vertex Colors", sceneMesh->mName.C_Str());
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

unsigned int ModelLoader::composeFlags(const ModelLoaderDesc& desc) {
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

std::shared_ptr<Texture> ModelLoader::loadTexture(const aiScene* scene, const aiString& texturePath,
                                                  const TextureLoaderDesc& desc,
                                                  ModelLoaderContext& context) {
  if (texturePath.length == 0) {
    spdlog::warn("ModelLoader: empty texture path");
    return nullptr;
  }

  const std::string pathStr = texturePath.C_Str();

  // Check if already loaded
  auto foundTexture = context.textureCache.find(pathStr);
  if (foundTexture != context.textureCache.end()) {
    spdlog::trace("ModelLoader textureCache hit: {}", pathStr);
    return foundTexture->second;
  }

  // 1) External files - relative to model, then global
  if (!pathStr.empty() && pathStr.front() != '*') {
    const auto modelDir = std::filesystem::path(context.modelPath).parent_path();
    const auto candidate = (modelDir / pathStr).generic_string();

    if (auto rel = assets::find(candidate)) {
      auto texture = TextureLoader::load(*rel, desc);
      context.textureCache[pathStr] = texture;
      return texture;
    }
    if (auto direct = assets::find(pathStr)) {
      auto texture = TextureLoader::load(*direct, desc);
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
      spdlog::error("ModelLoader: invalid embedded texture index '{}'", pathStr);
      return nullptr;
    }

    if (index < 0 || static_cast<unsigned>(index) >= scene->mNumTextures) {
      spdlog::error("ModelLoader: embedded texture index out of range ({})", index);
      return nullptr;
    }

    const std::span<aiTexture* const> sceneTextures{scene->mTextures, scene->mNumTextures};
    const aiTexture* sceneTexture = sceneTextures[index];
    if (sceneTexture == nullptr) {
      spdlog::error("ModelLoader: embedded texture {} is null", index);
      return nullptr;
    }

    // If mHeight is 0 - Compressed (Encoded Blob PNG/JPG/HDR/etc.)
    if (sceneTexture->mHeight == 0) {
      const auto* raw = std::bit_cast<const unsigned char*>(sceneTexture->pcData);
      const size_t size = sceneTexture->mWidth; // Assimp stores byte count in mWidth.
      auto texture = TextureLoader::loadFromMemory(raw, size, desc);
      context.textureCache[pathStr] = texture;
      return texture;
    }

    // if mHeight not 0 - Uncompressed RGBA8 array of aiTexel
    const int width = static_cast<int>(sceneTexture->mWidth);
    const int height = static_cast<int>(sceneTexture->mHeight);
    const size_t texelCount = static_cast<size_t>(width) * static_cast<size_t>(height);

    const std::span<const aiTexel> texels{sceneTexture->pcData, texelCount};
    std::vector<unsigned char> rgba;
    rgba.reserve(texelCount * 4);
    for (const aiTexel& texel : texels) {
      rgba.push_back(texel.r);
      rgba.push_back(texel.g);
      rgba.push_back(texel.b);
      rgba.push_back(texel.a);
    }
    auto texture = TextureLoader::fromRgba8(width, height, rgba.data(), desc);
    context.textureCache[pathStr] = texture;
    return texture;
  }

  spdlog::error("ModelLoader: texture not found '{}'", pathStr);
  return nullptr;
}

std::unique_ptr<Group> ModelLoader::makeFallbackModel() {
  auto group = std::make_unique<Group>();
  // ...
  return group;
}

} // namespace blkhurst
