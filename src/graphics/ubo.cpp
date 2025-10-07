#include <blkhurst/graphics/ubo.hpp>

#include <cassert>
#include <glad/gl.h>

namespace blkhurst {

UBO::UBO(unsigned bindingIndex)
    : binding_(bindingIndex) {
  glCreateBuffers(1, &id_);
}

UBO::~UBO() {
  if (id_ != 0U) {
    glDeleteBuffers(1, &id_);
    id_ = 0;
  }
}

unsigned UBO::id() const {
  return id_;
}

unsigned UBO::binding() const {
  return binding_;
}

std::size_t UBO::capacityBytes() const {
  return capacity_;
}

void UBO::bind() const {
  glBindBufferBase(GL_UNIFORM_BUFFER, binding_, id_);
}

void UBO::setData(const void* data, size_t sizeBytes) {
  // Only reallocate if new size is larger than current capacity
  if (sizeBytes > capacity_) {
    glNamedBufferData(id_, static_cast<GLsizeiptr>(sizeBytes), data, GL_DYNAMIC_DRAW);
    capacity_ = sizeBytes;
  } else {
    glNamedBufferSubData(id_, 0, static_cast<GLsizeiptr>(sizeBytes), data);
  }
}

void UBO::setSubData(size_t offsetBytes, const void* data, size_t sizeBytes) const {
  assert(offsetBytes + sizeBytes <= capacity_ && "UBO::setSubData out of bounds");
  glNamedBufferSubData(id_, static_cast<GLintptr>(offsetBytes), static_cast<GLsizeiptr>(sizeBytes),
                       data);
}

void UBO::getData(void* data, size_t sizeBytes) const {
  assert(sizeBytes <= capacity_ && "UBO::getData size exceeds buffer capacity");
  glGetNamedBufferSubData(id_, 0, static_cast<GLsizeiptr>(sizeBytes), data);
}

void UBO::getSubData(size_t offsetBytes, void* data, size_t sizeBytes) const {
  assert(offsetBytes + sizeBytes <= capacity_ && "UBO::getSubData out of bounds");
  glGetNamedBufferSubData(id_, static_cast<GLintptr>(offsetBytes),
                          static_cast<GLsizeiptr>(sizeBytes), data);
}

} // namespace blkhurst
