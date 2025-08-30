#include "graphics/buffer.hpp"
#include <glad/gl.h>
#include <spdlog/spdlog.h>

namespace blkhurst {

Buffer::Buffer(const void* data, GLsizeiptr sizeBytes, bool dynamic)
    : size_(sizeBytes) {
  glCreateBuffers(1, &id_);
  glNamedBufferData(id_, sizeBytes, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
  spdlog::trace("Buffer({}) created size={}B dynamic={}", id_, sizeBytes, dynamic);
}

Buffer::~Buffer() {
  if (id_ != 0U) {
    glDeleteBuffers(1, &id_);
    spdlog::trace("Buffer({}) deleted", id_);
    id_ = 0;
  }
}

void Buffer::setData(const void* data, GLsizeiptr sizeBytes, bool dynamic) {
  size_ = sizeBytes;
  glNamedBufferData(id_, sizeBytes, data, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
  spdlog::trace("Buffer({}) setData size={}B dynamic={}", id_, sizeBytes, dynamic);
}

void Buffer::setSubData(GLintptr offsetBytes, const void* data, GLsizeiptr sizeBytes) {
  assert(offsetBytes + sizeBytes <= size_ && "setSubData out of range");
  glNamedBufferSubData(id_, offsetBytes, sizeBytes, data);
  spdlog::trace("Buffer({}) setSubData offset={}B size={}B", id_, offsetBytes, sizeBytes);
}

} // namespace blkhurst
