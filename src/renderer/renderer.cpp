#include <blkhurst/materials/material.hpp>
#include <blkhurst/renderer/renderer.hpp>

#include <glad/gl.h>
#include <spdlog/spdlog.h>
#include <vector>

namespace blkhurst {

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

  for (auto* mesh : meshList) {
    renderMesh(*mesh, camera);
  }
}

void Renderer::setAutoClear(bool enabled) {
  autoClear_ = enabled;
}

void Renderer::setClearColor(glm::vec3 rgb) {
  clearColor_ = rgb;
  glClearColor(rgb[0], rgb[1], rgb[2], 1);
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
  const auto program = material ? material->program : nullptr;
  if (!geometry || !material || !program) {
    spdlog::warn("Renderer: Mesh missing Geometry/Material/Program");
    return;
  }

  // Apply PipelineState and use shader Program.
  applyPipeline(material->pipeline, mesh.wireframe());
  program->use();

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
  auto program = material->program;

  // Per-frame Uniforms
  // TODO: Moveto applyPerFrameUniforms with UBO
  program->setUniform("uTime", frameUniforms_.uTime);
  program->setUniform("uDelta", frameUniforms_.uDelta);
  program->setUniform("uMouse", frameUniforms_.uMouse);
  program->setUniform("uResolution", frameUniforms_.uResolution);
  program->setUniform("uView", frameUniforms_.uView);
  program->setUniform("uProjection", frameUniforms_.uProjection);
  program->setUniform("uCameraPos", frameUniforms_.uCameraPos);

  // Per-draw Uniforms
  material->program->setUniform("uModel", mesh.worldMatrix());

  // User Uniforms
  material->applyUniforms();
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

// ------- Helpers -------
unsigned Renderer::toGlPrimitive(PrimitiveMode mode) {
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

} // namespace blkhurst
