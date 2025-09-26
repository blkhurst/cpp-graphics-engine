#include <algorithm>
#include <blkhurst/geometry/geometry.hpp>
#include <cassert>
#include <glm/gtc/type_ptr.hpp>
#include <span>
#include <spdlog/spdlog.h>

namespace {
constexpr bool kDynamic = false;
}

namespace blkhurst {

Geometry::Geometry() {
  spdlog::trace("Geometry constructed");
}

Geometry::~Geometry() {
  spdlog::trace("Geometry destroyed");
}

std::shared_ptr<Geometry> Geometry::create() {
  return std::make_shared<Geometry>();
}

void Geometry::setAttribute(Attrib attrib, std::span<const float> data, int componentCount) {
  const auto attribIndex = static_cast<unsigned int>(attrib);

  auto vbo = std::make_unique<Buffer>(data, kDynamic);
  vao_.linkPackedFloatBuffer(attribIndex, vbo->id(), componentCount);
  vbos_.push_back(std::move(vbo));

  if (attrib == Attrib::Position) {
    const int vertexCount = static_cast<int>(data.size() / componentCount);
    vertexCount_ = vertexCount;

    // Only set draw count if not indexed
    if (!isIndexed_) {
      drawRange_.count = vertexCount;
    }
  }
  // TODO: Attribute count mismatch
}

void Geometry::setIndex(std::span<const unsigned> indices) {
  ebo_ = std::make_unique<Buffer>(indices, kDynamic);
  vao_.setElementBuffer(ebo_->id());

  isIndexed_ = true;
  const int indexCount = static_cast<int>(indices.size());
  indexCount_ = indexCount;
  drawRange_.start = 0;
  drawRange_.count = indexCount;
}

void Geometry::setPrimitive(PrimitiveMode mode) {
  primitive_ = mode;
  spdlog::trace("Geometry setPrimitive {}", static_cast<int>(mode));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Geometry::setDrawRange(int start, int count) {
  drawRange_.start = std::max(0, start);
  drawRange_.count = std::max(0, count);
  spdlog::trace("Geometry setDrawRange start={} count={}", drawRange_.start, drawRange_.count);
}

void Geometry::clearDrawRange() {
  drawRange_.start = 0;
  drawRange_.count = isIndexed_ ? indexCount_ : vertexCount_;
  spdlog::trace("Geometry clearDrawRange -> start={} count={}", drawRange_.start, drawRange_.count);
}

PrimitiveMode Geometry::primitive() const {
  return primitive_;
}

DrawRange Geometry::drawRange() const {
  return drawRange_;
}

bool Geometry::isIndexed() const {
  return isIndexed_;
}

const VertexArray& Geometry::vertexArray() const {
  return vao_;
}

std::shared_ptr<Geometry> Geometry::from(const MeshData& meshData) {
  auto geometry = Geometry::create();

  geometry->setIndex(meshData.indices);
  geometry->setAttribute(Attrib::Position, meshData.positions, 3);
  geometry->setAttribute(Attrib::Uv, meshData.uvs, 2);
  geometry->setAttribute(Attrib::Normal, meshData.normals, 3);

  return geometry;
}
} // namespace blkhurst
