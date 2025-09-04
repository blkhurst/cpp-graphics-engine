#pragma once

#include <cstdint>

namespace blkhurst {

class VertexArray {
public:
  VertexArray();
  ~VertexArray();

  VertexArray(const VertexArray&) = delete;
  VertexArray(VertexArray&&) = delete;
  VertexArray& operator=(const VertexArray&) = delete;
  VertexArray& operator=(VertexArray&&) = delete;

  void bind() const;
  static void unbind();

  void bindVertexBuffer(unsigned int bindingIndex, unsigned int bufferId, intptr_t offset,
                        int stride) const;
  void linkAttribFloat(unsigned int attribIndex, unsigned int bindingIndex, int componentCount,
                       bool normalised = false, unsigned int relativeOffset = 0) const;
  void setElementBuffer(unsigned int bufferId);

  // Convenience; single attribute per binding, packed floats
  void linkPackedFloatBuffer(unsigned int index, unsigned int bufferId, int componentCount) const;

private:
  unsigned int id_ = 0;
};

} // namespace blkhurst
