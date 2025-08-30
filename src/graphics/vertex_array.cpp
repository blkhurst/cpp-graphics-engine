#include "graphics/vertex_array.hpp"
#include <glad/gl.h>
#include <spdlog/spdlog.h>

namespace blkhurst {

VertexArray::VertexArray() {
  glCreateVertexArrays(1, &id_);
  spdlog::trace("VertexArray({}) created", id_);
}

VertexArray::~VertexArray() {
  if (id_ != 0U) {
    glDeleteVertexArrays(1, &id_);
    spdlog::trace("VertexArray({}) deleted", id_);
    id_ = 0;
  }
}

void VertexArray::bind() const {
  glBindVertexArray(id_);
}

void VertexArray::unbind() {
  glBindVertexArray(0);
}

void VertexArray::bindVertexBuffer(GLuint bindingIndex, GLuint bufferId, GLintptr offset,
                                   GLsizei stride) const {
  glVertexArrayVertexBuffer(id_, bindingIndex, bufferId, offset, stride);
  spdlog::trace("VertexArray({}) binds Buffer({}) at binding={} offset={} stride={}", id_, bufferId,
                bindingIndex, offset, stride);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void VertexArray::linkAttribFloat(GLuint attribIndex, GLuint bindingIndex, GLint componentCount,
                                  bool normalised, GLuint relativeOffset) const {
  assert(componentCount >= 1 && componentCount <= 4);
  const GLboolean norm = normalised ? GL_TRUE : GL_FALSE;

  glEnableVertexArrayAttrib(id_, attribIndex);
  glVertexArrayAttribBinding(id_, attribIndex, bindingIndex);
  glVertexArrayAttribFormat(id_, attribIndex, componentCount, GL_FLOAT, norm, relativeOffset);
  spdlog::trace(
      "VertexArray({}) links attrib={} to binding={} | count={} normalised={} relOffset={}", id_,
      attribIndex, bindingIndex, componentCount, normalised, relativeOffset);
}

void VertexArray::setElementBuffer(GLuint bufferId) {
  glVertexArrayElementBuffer(id_, bufferId);
  spdlog::trace("VertexArray({}) set ElementBuffer({})", id_, bufferId);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void VertexArray::linkPackedFloatBuffer(GLuint index, GLuint bufferId, GLint componentCount) const {
  const auto stride = static_cast<GLsizei>(componentCount * sizeof(GLfloat));

  bindVertexBuffer(index, bufferId, 0, stride);
  linkAttribFloat(index, index, componentCount);
}

} // namespace blkhurst
