#include <blkhurst/geometry/box_geometry.hpp>
#include <blkhurst/lights/ambient_light.hpp>
#include <blkhurst/lights/directional_light.hpp>
#include <blkhurst/lights/light.hpp>
#include <blkhurst/lights/point_light.hpp>
#include <blkhurst/materials/material.hpp>
#include <blkhurst/materials/skybox_material.hpp>
#include <blkhurst/renderer/cube_render_target.hpp>
#include <blkhurst/renderer/renderer.hpp>
#include <blkhurst/scene/scene.hpp>

#include <glad/gl.h>
#include <spdlog/spdlog.h>
#include <vector>

namespace blkhurst {

Renderer::Renderer() {
  auto backgroundGeom = BoxGeometry::create({.width = 2.0F, .height = 2.0F, .depth = 2.0F});
  auto backgroundMat = SkyBoxMaterial::create();
  skyboxMesh_ = Mesh::create(backgroundGeom, backgroundMat);
  skyboxMesh_->setName("RendererSkyBox");

  spdlog::debug("Renderer constructed");
}

void Renderer::setFrameUniforms(const FrameUniforms& frameUniforms) {
  frameUniforms_ = frameUniforms;
}

void Renderer::setRenderTarget(const RenderTarget* target) {
  bool bindDefaultFramebuffer = (target == nullptr);
  if (bindDefaultFramebuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    setViewport(0, 0, framebufferSize_[0], framebufferSize_[1]);
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, target->id());
  setViewport(0, 0, target->width(), target->height());
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Renderer::setRenderTarget(const CubeRenderTarget* target, int face, int mip) {
  bool bindDefaultFramebuffer = (target == nullptr);
  if (bindDefaultFramebuffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    setViewport(0, 0, framebufferSize_[0], framebufferSize_[1]);
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

void Renderer::render(Object3D& root, Camera& camera) {
  if (autoClear_) {
    clear();
  }

  FrameContext frameContext;
  frameContext = collectRenderables(root);

  applyPerFrameUniforms(frameContext);

  if (auto* scene = dynamic_cast<Scene*>(&root)) {
    setEnvironment(*scene);
    renderBackground(*scene, camera);
  }

  for (auto* mesh : frameContext.meshList) {
    renderMesh(*mesh, camera);
  }
}

void Renderer::setAutoClear(bool enabled) {
  autoClear_ = enabled;
}

void Renderer::setClearColor(glm::vec4 rgba) {
  clearColor_ = rgba;
  glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Renderer::clear(bool color, bool depth, bool stencil) {
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
  glClear(mask);
}

void Renderer::clearColor() {
  clear(true, false, false);
}

void Renderer::clearDepth() {
  clear(false, true, false);
}

void Renderer::clearStencil() {
  clear(false, false, true);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Renderer::setDefaultFramebufferSize(int width, int height) {
  framebufferSize_[0] = width;
  framebufferSize_[1] = height;
  setViewport(0, 0, width, height);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Renderer::setViewport(int xpos, int ypos, int width, int height) {
  glViewport(xpos, ypos, width, height);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Renderer::setScissor(int xpos, int ypos, int width, int height) {
  glScissor(xpos, ypos, width, height);
}

void Renderer::setScissorTest(bool enabled) {
  scissorTestEnabled_ = enabled;
  if (enabled) {
    glEnable(GL_SCISSOR_TEST);
  } else {
    glDisable(GL_SCISSOR_TEST);
  }
}

void Renderer::setToneMappingExposure(float exposure) {
  toneMappingExposure_ = exposure;
}

void Renderer::setToneMappingMode(ToneMappingMode mode) {
  toneMappingMode_ = mode;
}

void Renderer::setOutputColorSpace(OutputColorSpace space) {
  outputColorSpace_ = space;
}

void Renderer::resetState() {
  autoClear_ = true;
  clearColor_ = defaults::window::clearColor;
  setClearColor(clearColor_);

  setRenderTarget(nullptr);
  setScissorTest(false);

  spdlog::debug("Renderer state reset");
}

FrameContext Renderer::collectRenderables(Object3D& root) {
  // FrameContext must be locally owned to prevent renderer clearing queue in nested renders.
  FrameContext context;

  root.traverse([&](Object3D& node) {
    if (!node.visible()) {
      return;
    }

    if (node.type() == NodeType::Mesh) {
      auto* mesh = dynamic_cast<Mesh*>(&node);
      context.meshList.push_back(mesh);
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

void Renderer::applyPerFrameUniforms(const FrameContext& frameContext) {
  frameUniforms_.uToneMappingExposure = toneMappingExposure_;
  frameUniforms_.uToneMappingMode = static_cast<int>(toneMappingMode_);
  frameUniforms_.uOutputColorSpace = static_cast<int>(outputColorSpace_);

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
  material.setUniform("uModel", mesh.worldMatrix());

  // Apply Uniforms & Resources
  material.applyEnvironment(environmentBundle_);
  material.applyUniformsAndResources(); // Apply last - flushes pending setUniform calls
}

void Renderer::drawGeometry(const Geometry& geom, int instanceCount) {
  const DrawRange range = geom.drawRange();
  const GLenum primitive = toGlPrimitive(geom.primitive());

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
    clearColor();
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

  if (sceneEnvironment.needsUpdate) {
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
