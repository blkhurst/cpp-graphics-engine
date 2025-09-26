#pragma once

#include <blkhurst/geometry/mesh_data.hpp>
#include <blkhurst/graphics/buffer.hpp>
#include <blkhurst/graphics/vertex_array.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace blkhurst {

enum class PrimitiveMode : std::uint8_t { Triangles, Lines, Points };

struct DrawRange {
  int start = 0;
  int count = 0;
};

// Tangent Unused, TBN calculated via dFdx/dfdy
enum class Attrib : std::uint8_t {
  Position = 0,
  Color = 1,
  Uv = 2,
  Normal = 3,
  InstanceColor = 4,
  InstanceMatrix = 5,
};

class Geometry {
public:
  Geometry();
  virtual ~Geometry();

  Geometry(const Geometry&) = delete;
  Geometry& operator=(const Geometry&) = delete;
  Geometry(Geometry&&) = delete;
  Geometry& operator=(Geometry&&) = delete;

  static std::shared_ptr<Geometry> create();

  void setAttribute(Attrib attrib, std::span<const float> data, int componentCount);
  void setIndex(std::span<const unsigned> indices);

  void setPrimitive(PrimitiveMode mode);
  void setDrawRange(int start, int count);
  void clearDrawRange();

  [[nodiscard]] PrimitiveMode primitive() const;
  [[nodiscard]] DrawRange drawRange() const;
  [[nodiscard]] bool isIndexed() const;
  [[nodiscard]] const VertexArray& vertexArray() const;

  static std::shared_ptr<Geometry> from(const MeshData& meshData);

private:
  VertexArray vao_;
  // Geometry owns Buffer; one attribute per Buffer
  std::vector<std::unique_ptr<Buffer>> vbos_;
  std::unique_ptr<Buffer> ebo_;

  PrimitiveMode primitive_ = PrimitiveMode::Triangles;
  DrawRange drawRange_;
  bool isIndexed_ = false;

  // Cache for clearDrawRange
  int vertexCount_ = 0;
  int indexCount_ = 0;
};

} // namespace blkhurst
