#include <blkhurst/graphics/ssbo.hpp>

#include <cassert>
#include <glad/gl.h>

namespace blkhurst {

SSBO::SSBO(unsigned bindingIndex)
    : binding_(bindingIndex) {
  glCreateBuffers(1, &id_);
}

SSBO::~SSBO() {
  if (id_ != 0U) {
    glDeleteBuffers(1, &id_);
    id_ = 0;
  }
}

unsigned SSBO::id() const {
  return id_;
}

unsigned SSBO::binding() const {
  return binding_;
}

std::size_t SSBO::capacityBytes() const {
  return capacity_;
}

void SSBO::bind() const {
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_, id_);
}

void SSBO::barrier() {
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
}

void SSBO::setData(const void* data, size_t sizeBytes) {
  // Only reallocate if new size is larger than current capacity
  if (sizeBytes > capacity_) {
    glNamedBufferData(id_, static_cast<GLsizeiptr>(sizeBytes), data, GL_DYNAMIC_DRAW);
    capacity_ = sizeBytes;
  } else {
    glNamedBufferSubData(id_, 0, static_cast<GLsizeiptr>(sizeBytes), data);
  }
}

void SSBO::setSubData(size_t offsetBytes, const void* data, size_t sizeBytes) const {
  assert(offsetBytes + sizeBytes <= capacity_ && "SSBO::setSubData out of bounds");
  glNamedBufferSubData(id_, static_cast<GLintptr>(offsetBytes), static_cast<GLsizeiptr>(sizeBytes),
                       data);
}

void SSBO::getData(void* data, size_t sizeBytes) const {
  assert(sizeBytes <= capacity_ && "SSBO::getData size exceeds buffer capacity");
  glGetNamedBufferSubData(id_, 0, static_cast<GLsizeiptr>(sizeBytes), data);
}

void SSBO::getSubData(size_t offsetBytes, void* data, size_t sizeBytes) const {
  assert(offsetBytes + sizeBytes <= capacity_ && "SSBO::getSubData out of bounds");
  glGetNamedBufferSubData(id_, static_cast<GLintptr>(offsetBytes),
                          static_cast<GLsizeiptr>(sizeBytes), data);
}

} // namespace blkhurst
