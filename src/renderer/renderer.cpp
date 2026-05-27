#include <blkhurst/geometry/box_geometry.hpp>
#include <blkhurst/lights/ambient_light.hpp>
#include <blkhurst/lights/directional_light.hpp>
#include <blkhurst/lights/light.hpp>
#include <blkhurst/lights/point_light.hpp>
#include <blkhurst/materials/material.hpp>
#include <blkhurst/materials/skybox_material.hpp>
#include <blkhurst/materials/uniforms.hpp>
#include <blkhurst/renderer/cube_render_target.hpp>
#include <blkhurst/renderer/renderer.hpp>
#include <blkhurst/scene/scene.hpp>
#include <blkhurst/util/color.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glad/gl.h>
#include <glm/gtx/norm.hpp>
#include <spdlog/spdlog.h>
#include <vector>

namespace blkhurst {

Renderer::Renderer() {
  glGenQueries(1, &statsGpuTimerQueryId_);

  auto backgroundGeom = BoxGeometry::create({.width = 2.0F, .height = 2.0F, .depth = 2.0F});
  auto backgroundMat = SkyBoxMaterial::create();
  skyboxMesh_ = Mesh::create(backgroundGeom, backgroundMat);
  skyboxMesh_->setName("RendererSkyBox");

  spdlog::debug("Renderer constructed");
}

Renderer::~Renderer() {
  if (statsGpuTimerActive_) {
    glEndQuery(GL_TIME_ELAPSED);
  }
  glDeleteQueries(1, &statsGpuTimerQueryId_);
}

const RendererDesc& Renderer::desc() const {
  return desc_;
}

const RendererStats& Renderer::stats() const {
  return stats_;
}

void Renderer::render(Object3D& root, Camera& camera) {
  beginPass();

  FrameContext frameContext;
  frameContext = collectRenderables(root, camera);

  applyPerFrameUniforms(frameContext);

  if (auto* scene = dynamic_cast<Scene*>(&root)) {
    setEnvironment(*scene);
    renderBackground(*scene, camera);
  }

  // Render Opaque Meshes
  for (auto* mesh : frameContext.opaqueMeshes) {
    renderMesh(*mesh, camera);
  }
  // Render Transparent Meshes
  for (auto* mesh : frameContext.transparentMeshes) {
    renderMesh(*mesh, camera);
  }
}

// Set Known-Safe Baseline State Per Pass
void Renderer::beginPass() {
  stats_.passes++;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDisable(GL_BLEND);

  // Ensure Buffers Are Writeable & Clear
  if (desc_.autoClear) {
    clear(true, true, true);
  }
}

void Renderer::beginFrameStats() {
  stats_ = {};
  statsCpuStart_ = std::chrono::steady_clock::now();
  if (statsGpuTimerActive_) {
    glEndQuery(GL_TIME_ELAPSED);
  }
  glBeginQuery(GL_TIME_ELAPSED, statsGpuTimerQueryId_);
  statsGpuTimerActive_ = true;
}

void Renderer::endFrameStats() {
  const auto cpuEnd = std::chrono::steady_clock::now();
  stats_.cpuMs = std::chrono::duration<float, std::milli>(cpuEnd - statsCpuStart_).count();

  if (!statsGpuTimerActive_) {
    return;
  }

  glEndQuery(GL_TIME_ELAPSED);
  statsGpuTimerActive_ = false;

  GLuint64 gpuNs = 0;
  const int NsPerMs = 1'000'000;
  glGetQueryObjectui64v(statsGpuTimerQueryId_, GL_QUERY_RESULT, &gpuNs);
  stats_.gpuMs = static_cast<float>(static_cast<double>(gpuNs) / NsPerMs);
}

void Renderer::setFrameUniforms(const FrameUniforms& frameUniforms) {
  frameUniforms_ = frameUniforms;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Renderer::setDefaultFramebufferSize(int width, int height) {
  desc_.framebufferSize[0] = width;
  desc_.framebufferSize[1] = height;
  setViewport(0, 0, width, height);
}

void Renderer::setRenderTarget(const RenderTarget* target) {
  desc_.currentTarget_ = target;

  bool bindDefaultFramebuffer = (target == nullptr);
  if (bindDefaultFramebuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    resetViewport();
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, target->id());
  resetViewport();
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Renderer::setRenderTarget(const CubeRenderTarget* target, int face, int mip) {
  // TODO: Merge RenderTarget and CubeRenderTarget for unified "currentTarget_"
  bool bindDefaultFramebuffer = (target == nullptr);
  if (bindDefaultFramebuffer) {
    desc_.currentTarget_ = nullptr; // TODO Move Upwards & Set To "target"
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    resetViewport();
    return;
  }

  const unsigned framebufferId = target->id();
  glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

  // Attach Color face/mip
  const unsigned textureId = target->texture()->id();
  glNamedFramebufferTextureLayer(framebufferId, GL_COLOR_ATTACHMENT0, textureId, mip, face);

  // Attach Depth face/mip
  if (auto depthTexture = target->depthTexture()) {
    const auto format = depthTexture->desc().format;
    const GLenum attachment =
        Texture::isDepthStencilFormat(format) ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
    glNamedFramebufferTextureLayer(framebufferId, attachment, depthTexture->id(), mip, face);
  }

  // Viewport for this mip level
  const int size = std::max(1, target->size() >> mip);
  setViewport(0, 0, size, size);
}

void Renderer::setAutoClear(bool enabled) {
  desc_.autoClear = enabled;
}

void Renderer::setClearColor(glm::vec4 rgba) {
  desc_.clearColor = rgba;
}

void applyClearColorForTarget(const RendererDesc& desc) {
  glm::vec4 rgba = desc.clearColor;

  if (desc.currentTarget_ != nullptr) {
    rgba = color::srgbToLinear(rgba);
  }

  glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Renderer::clear(bool color, bool depth, bool stencil) {
  // Ensure Buffers are Writable
  glDisable(GL_SCISSOR_TEST);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDepthMask(GL_TRUE);
  glStencilMask(0xFF);

  GLbitfield mask = 0;
  if (color) {
    mask |= GL_COLOR_BUFFER_BIT;
  }
  if (depth) {
    mask |= GL_DEPTH_BUFFER_BIT;
  }
  if (stencil) {
    mask |= GL_STENCIL_BUFFER_BIT;
  }

  applyClearColorForTarget(desc_);
  glClear(mask);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Renderer::setViewport(int xpos, int ypos, int width, int height) {
  glViewport(xpos, ypos, width, height);
}

void Renderer::resetViewport() {
  if (desc_.currentTarget_ == nullptr) {
    setViewport(0, 0, desc_.framebufferSize[0], desc_.framebufferSize[1]);
    return;
  }
  setViewport(0, 0, desc_.currentTarget_->width(), desc_.currentTarget_->height());
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Renderer::setScissor(int xpos, int ypos, int width, int height) {
  glScissor(xpos, ypos, width, height);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Renderer::setScissorTest(bool enabled) {
  if (enabled) {
    glEnable(GL_SCISSOR_TEST);
  } else {
    glDisable(GL_SCISSOR_TEST);
  }
}

void Renderer::setToneMappingExposure(float exposure) {
  desc_.toneMappingExposure = exposure;
}

void Renderer::setToneMappingMode(ToneMappingMode mode) {
  desc_.toneMappingMode = mode;
}

void Renderer::setOutputColorSpace(OutputColorSpace space) {
  desc_.outputColorSpace = space;
}

void Renderer::resetState() {
  RendererDesc defaultDesc;
  defaultDesc.framebufferSize = desc_.framebufferSize;
  desc_ = defaultDesc;

  setClearColor(desc_.clearColor);

  setRenderTarget(nullptr);
  setScissorTest(false);

  setToneMappingExposure(1.0);
  setToneMappingMode(ToneMappingMode::None);
  setOutputColorSpace(OutputColorSpace::SRGB);

  spdlog::debug("Renderer state reset");
}

FrameContext Renderer::collectRenderables(Object3D& root, const Camera& camera) {
  // FrameContext must be locally owned to prevent renderer clearing queue in nested renders.
  FrameContext context;
  const glm::vec3 cameraPos = camera.worldPosition();

  root.traverse([&](Object3D& node) {
    if (!node.visible()) {
      return;
    }

    if (node.type() == NodeType::Mesh) {
      auto* mesh = dynamic_cast<Mesh*>(&node);
      const auto material = mesh->material();

      const bool isTransparent = material->pipeline().blend;
      isTransparent ? context.transparentMeshes.push_back(mesh)
                    : context.opaqueMeshes.push_back(mesh);
    }

    if (node.type() == NodeType::Light) {
      auto* light = dynamic_cast<Light*>(&node);

      if (light->type() == LightType::Ambient) {
        context.lightData.ambientColor += light->color();
        context.lightData.ambientIntensity += light->intensity();
      }

      if (light->type() == LightType::Directional) {
        auto* dirLight = dynamic_cast<DirectionalLight*>(light);
        context.directionalLights.push_back(
            {dirLight->color(), dirLight->intensity(), dirLight->directionToTarget()});
        context.lightData.directionalCount++;
      }

      if (light->type() == LightType::Point) {
        auto* pointLight = dynamic_cast<PointLight*>(light);
        context.pointLights.push_back({pointLight->color(), pointLight->intensity(),
                                       pointLight->worldPosition(), pointLight->decay(),
                                       pointLight->distance()});
        context.lightData.pointCount++;
      }
    }
  });

  // Sort Opaque Front-To-Back
  std::sort(context.opaqueMeshes.begin(), context.opaqueMeshes.end(),
            [&](const Mesh* meshA, const Mesh* meshB) {
              // distance^2 — avoids sqrt
              const float distA = glm::distance2(cameraPos, meshA->worldPosition());
              const float distB = glm::distance2(cameraPos, meshB->worldPosition());
              return distA < distB;
            });

  // Sort Transparent Back-To-Front
  std::sort(context.transparentMeshes.begin(), context.transparentMeshes.end(),
            [&](const Mesh* meshA, const Mesh* meshB) {
              const float distA = glm::distance2(cameraPos, meshA->worldPosition());
              const float distB = glm::distance2(cameraPos, meshB->worldPosition());
              return distA > distB;
            });

  return context;
}

void Renderer::renderMesh(const Mesh& mesh, const Camera& camera) {
  const auto geometry = mesh.geometry();
  const auto material = mesh.material();
  if (!geometry || !material) {
    spdlog::warn("Renderer: Mesh missing Geometry/Material");
    return;
  }

  // TODO: Note:
  // useProgram then applyPerDrawUniforms
  //  - If define changes within applyPerDrawUniforms (USE_IBL), wont be picked up until next frame
  // applyPerDrawUniforms then useProgram
  //  - DSA allows this, but first frame after a rebuild, uniforms are lost (set on old programID)
  // Order should be setDefines, useProgram (Rebuild), applyUniforms

  // Apply Instancing (sets defines)
  applyInstancing(mesh, *geometry, *material);

  // Apply PipelineState and use shader Program.
  applyPipeline(material->pipeline(), mesh.wireframe());
  material->useProgram();

  // Per-draw Uniforms
  applyPerDrawUniforms(mesh, *material);

  // Bind VertexArray & Draw
  geometry->vertexArray().bind();
  drawGeometry(*geometry, mesh.instanceCount());
  VertexArray::unbind();
}

void Renderer::applyPipeline(const PipelineState& state, bool wireframe) {
  if (state.depthTest) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(toGLDepthFunc(state.depthFunc));
  } else {
    glDisable(GL_DEPTH_TEST);
  }

  glDepthMask(state.depthWrite ? GL_TRUE : GL_FALSE);

  if (state.blend) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    glDisable(GL_BLEND);
  }

  switch (state.cull) {
  case CullFace::Back:
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    break;
  case CullFace::Front:
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    break;
  case CullFace::None:
    glDisable(GL_CULL_FACE);
    break;
  }

  if (wireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  } else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
}

void Renderer::drawGeometry(const Geometry& geom, int instanceCount) {
  const DrawRange range = geom.drawRange();
  const GLenum primitive = toGlPrimitive(geom.primitive());

  // Update Stats
  stats_.drawCalls++;
  stats_.instances += instanceCount > 1 ? instanceCount : 0;
  if (geom.primitive() == PrimitiveMode::Triangles) {
    stats_.triangles += (range.count / 3) * instanceCount;
  }

  if (geom.primitive() == PrimitiveMode::Patches) {
    glPatchParameteri(GL_PATCH_VERTICES, geom.patchVertices());
  }

  if (geom.isIndexed()) {
    auto offsetBytes = range.start * sizeof(std::uint32_t);
    const void* indexOffset = std::bit_cast<const void*>(offsetBytes);

    if (instanceCount > 1) {
      glDrawElementsInstanced(primitive, range.count, GL_UNSIGNED_INT, indexOffset, instanceCount);
    } else {
      glDrawElements(primitive, range.count, GL_UNSIGNED_INT, indexOffset);
    }
  } else {
    if (instanceCount > 1) {
      glDrawArraysInstanced(primitive, range.start, range.count, instanceCount);
    } else {
      glDrawArrays(primitive, range.start, range.count);
    }
  }
}

void Renderer::applyPerFrameUniforms(const FrameContext& frameContext) {
  // Only Apply ColorSpace / ToneMapping When Rendering To Default Framebuffer
  // https://github.com/mrdoob/three.js/blob/02201339d5429a610a71ec19f5bf36eb4e7d2b04/src/renderers/WebGLRenderer.js#L2198
  if (desc_.currentTarget_ == nullptr) {
    frameUniforms_.uOutputColorSpace = static_cast<int>(desc_.outputColorSpace);
    frameUniforms_.uToneMappingMode = static_cast<int>(desc_.toneMappingMode);
  } else {
    frameUniforms_.uOutputColorSpace = static_cast<int>(OutputColorSpace::Linear);
    frameUniforms_.uToneMappingMode = static_cast<int>(ToneMappingMode::None);
  }
  frameUniforms_.uToneMappingExposure = desc_.toneMappingExposure;

  // Frame Uniforms
  gpuBlocks_.frame.update(frameUniforms_);
  gpuBlocks_.frame.bind();

  // Light Data // TODO: lightsDirty_ flag
  gpuBlocks_.lightData.update(frameContext.lightData);
  gpuBlocks_.lightData.bind();

  // Directional Lights
  gpuBlocks_.directionalLights.updateArray(std::span{frameContext.directionalLights});
  gpuBlocks_.directionalLights.bind();

  // Point Lights
  gpuBlocks_.pointLights.updateArray(std::span{frameContext.pointLights});
  gpuBlocks_.pointLights.bind();
}

void Renderer::applyPerDrawUniforms(const Mesh& mesh, Material& material) const {
  // Per-draw Uniforms
  material.setUniform(uniforms::Model, mesh.worldMatrix());

  // Apply Uniforms & Resources
  material.applyEnvironment(environmentBundle_);
  material.applyUniformsAndResources(); // Apply last - flushes pending setUniform calls
}

void Renderer::applyInstancing(const Mesh& mesh, Geometry& geometry, Material& material) {
  const bool useInstancing = mesh.instanceCount() > 1;
  material.setDefine(defines::UseInstancing, useInstancing);
  material.setDefine(defines::UseInstanceColor, useInstancing && mesh.hasInstanceColors());

  if (!useInstancing) {
    return;
  }

  // TODO: Implement instancesDirty_ flag to avoid redundant re-uploads
  geometry.setInstanceMatrices(mesh.instanceMatrices());
  if (mesh.hasInstanceColors()) {
    geometry.setInstanceColors(mesh.instanceColors());
  }
}

void Renderer::renderBackground(Scene& scene, Camera& camera) {
  const auto& sceneBackground = scene.background();
  const auto& sceneEnvironment = scene.environment();

  if (!skyboxMesh_) {
    return;
  }

  auto* skyboxMaterial = dynamic_cast<SkyBoxMaterial*>(skyboxMesh_->material().get());
  if (skyboxMaterial == nullptr) {
    spdlog::warn("Renderer::renderBackground SkyBoxMaterial is null");
    return;
  }

  if (sceneBackground.type == BackgroundType::Color) {
    setClearColor(sceneBackground.color);
    return;
  }

  if (sceneBackground.type == BackgroundType::Equirect) {
    auto cubeRenderTarget = CubeRenderTarget::fromEquirect(*this, sceneBackground.texture);
    scene.setBackground(cubeRenderTarget->texture());
  }

  if (sceneBackground.type == BackgroundType::Cube) {
    skyboxMaterial->setCubeMap(sceneBackground.cubemap);
    skyboxMaterial->setCubeMapRotation(sceneEnvironment.rotation); // Controlled via envRotation
    skyboxMaterial->setIntensity(sceneBackground.intensity);

    renderMesh(*skyboxMesh_, camera);
  }
}

void Renderer::setEnvironment(Scene& scene) {
  const auto& sceneBackground = scene.background();
  auto& sceneEnvironment = scene.environment();

  if (sceneEnvironment.needsUpdate && sceneEnvironment.equirect) {
    // Convert Equirect to Cubemap
    auto cubemap = CubeRenderTarget::fromEquirect(*this, sceneEnvironment.equirect)->texture();

    // Generate PMREM
    auto pmremResult = pmremGenerator_.fromCubemap(cubemap);
    sceneEnvironment.brdfLUT = pmremResult.brdfLUT;
    sceneEnvironment.irradianceMap = pmremResult.irradianceMap;
    sceneEnvironment.prefilterMap = pmremResult.prefilterMap;
    sceneEnvironment.needsUpdate = false;

    // Set Background
    if (sceneEnvironment.setBackground) {
      scene.setBackground(cubemap);
    }
  }

  // Update EnvironmentBundle
  environmentBundle_.environmentMap = sceneBackground.cubemap;
  environmentBundle_.brdfLUT = sceneEnvironment.brdfLUT;
  environmentBundle_.irradianceMap = sceneEnvironment.irradianceMap;
  environmentBundle_.prefilterMap = sceneEnvironment.prefilterMap;
  environmentBundle_.rotation = sceneEnvironment.rotation;
  environmentBundle_.intensity = sceneEnvironment.intensity;
}

// ------- Helpers -------
GLenum Renderer::toGlPrimitive(PrimitiveMode mode) {
  switch (mode) {
  case PrimitiveMode::Triangles:
    return GL_TRIANGLES;
  case PrimitiveMode::Lines:
    return GL_LINES;
  case PrimitiveMode::Points:
    return GL_POINTS;
  case PrimitiveMode::Patches:
    return GL_PATCHES;
  default:
    return GL_TRIANGLES;
  }
}

GLenum Renderer::toGLDepthFunc(blkhurst::DepthFunc func) {
  using DF = blkhurst::DepthFunc;
  switch (func) {
  case DF::Never:
    return GL_NEVER;
  case DF::Less:
    return GL_LESS;
  case DF::Equal:
    return GL_EQUAL;
  case DF::Lequal:
    return GL_LEQUAL;
  case DF::Greater:
    return GL_GREATER;
  case DF::NotEqual:
    return GL_NOTEQUAL;
  case DF::Gequal:
    return GL_GEQUAL;
  case DF::Always:
    return GL_ALWAYS;
  }
  return GL_LESS;
}

} // namespace blkhurst
