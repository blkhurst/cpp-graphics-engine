#include <blkhurst/geometry/box_geometry.hpp>
#include <blkhurst/materials/material.hpp>
#include <blkhurst/materials/skybox_material.hpp>
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

  spdlog::trace("Renderer constructed");
}

void Renderer::setFrameUniforms(const FrameUniforms& frameUniforms) {
  frameUniforms_ = frameUniforms;
}

void Renderer::setRenderTarget(const RenderTarget* target) {
  if (target == currentTarget_) {
    return;
  }

  if (target == nullptr || target->fbo() == 0U) {
    RenderTarget::bindDefault();
    setViewport(0, 0, framebufferSize_[0], framebufferSize_[1]);
    currentTarget_ = target;
    return;
  }

  target->bind(); // Sets viewport internally
  currentTarget_ = target;
}

void Renderer::render(Object3D& root, Camera& camera) {
  if (autoClear_) {
    clear();
  }

  applyPerFrameUniforms(camera);

  // Build Node List
  std::vector<Mesh*> meshList;
  root.traverse([&](Object3D& node) {
    if (!node.visible()) {
      return;
    }
    if (node.kind() == NodeKind::Mesh) {
      auto* mesh = dynamic_cast<Mesh*>(&node);
      meshList.push_back(mesh);
    }
    if (node.kind() == NodeKind::Light) {
      // ...
    }
  });

  if (auto* scene = dynamic_cast<Scene*>(&root)) {
    renderBackground(*scene, camera);
  }

  for (auto* mesh : meshList) {
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

void Renderer::resetState() {
  currentTarget_ = nullptr;

  autoClear_ = true;
  clearColor_ = defaults::window::clearColor;
  setClearColor(clearColor_);

  setRenderTarget(nullptr);
  setScissorTest(false);

  spdlog::debug("Renderer state reset");
}

void Renderer::renderMesh(const Mesh& mesh, const Camera& camera) {
  const auto geometry = mesh.geometry();
  const auto material = mesh.material();
  if (!geometry || !material) {
    spdlog::warn("Renderer: Mesh missing Geometry/Material");
    return;
  }

  // Apply PipelineState and use shader Program.
  applyPipeline(material->pipeline(), mesh.wireframe());
  material->useProgram();

  // Per-draw Uniforms
  applyPerDrawUniforms(mesh, camera);

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

void Renderer::applyPerFrameUniforms(const Camera& camera) {
  // TODO: Update/bind UBO/SSBO if needsUpdate. Possible "global" textures (shadow, env, etc).
}

void Renderer::applyPerDrawUniforms(const Mesh& mesh, const Camera& camera) const {
  auto material = mesh.material();

  // Per-frame Uniforms
  // TODO: Moveto applyPerFrameUniforms with UBO
  material->setUniform("uTime", frameUniforms_.uTime);
  material->setUniform("uDelta", frameUniforms_.uDelta);
  material->setUniform("uMouse", frameUniforms_.uMouse);
  material->setUniform("uResolution", frameUniforms_.uResolution);
  material->setUniform("uView", frameUniforms_.uView);
  material->setUniform("uProjection", frameUniforms_.uProjection);
  material->setUniform("uCameraPos", frameUniforms_.uCameraPos);

  // Per-draw Uniforms
  material->setUniform("uModel", mesh.worldMatrix());

  // Apply Uniforms & Resources
  material->applyUniformsAndResources();
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
  // const auto& sceneEnvironment = scene.environment();

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

  switch (sceneBackground.type) {
  case BackgroundType::Cube:
    skyboxMaterial->setCubeMap(sceneBackground.cubemap);
    // skyboxMaterial->setCubeMapRotation(sceneEnvironment.rotation);
    skyboxMaterial->setIntensity(sceneBackground.intensity);
    break;
  default:
    return;
  }

  renderMesh(*skyboxMesh_, camera);
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
