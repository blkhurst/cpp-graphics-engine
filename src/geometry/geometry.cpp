#include <algorithm>
#include <blkhurst/geometry/geometry.hpp>
#include <cassert>
#include <glm/gtc/type_ptr.hpp>
#include <span>
#include <spdlog/spdlog.h>

namespace {
constexpr bool kDynamic = false;
constexpr bool kDynamicInstance = true;
} // namespace

namespace blkhurst {

Geometry::Geometry() {
  spdlog::trace("Geometry({}) constructed", uuidString());
}

Geometry::~Geometry() {
  spdlog::trace("Geometry({}) destroyed", uuidString());
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
    positions_.assign(data.begin(), data.end());

    const int vertexCount = static_cast<int>(data.size() / componentCount);
    vertexCount_ = vertexCount;

    // Only set draw count if not indexed
    if (!isIndexed_) {
      drawRange_.count = vertexCount;
    }
  }
  if (attrib == Attrib::Normal) {
    normals_.assign(data.begin(), data.end());
  }
  if (attrib == Attrib::Uv) {
    uvs_.assign(data.begin(), data.end());
  }
  // TODO: Attribute count mismatch
}

void Geometry::setIndex(std::span<const unsigned> indices) {
  indices_.assign(indices.begin(), indices.end());
  ebo_ = std::make_unique<Buffer>(indices, kDynamic);
  vao_.setElementBuffer(ebo_->id());

  isIndexed_ = true;
  const int indexCount = static_cast<int>(indices.size());
  indexCount_ = indexCount;
  drawRange_.start = 0;
  drawRange_.count = indexCount;
}

void Geometry::setInstanceMatrices(std::span<const glm::mat4> matrices) {
  if (!instanceMatrixBuffer_) {
    instanceMatrixBuffer_ = std::make_unique<Buffer>(matrices, kDynamicInstance);
  } else {
    instanceMatrixBuffer_->setData(matrices, kDynamicInstance);
  }

  constexpr auto bindingIndex = static_cast<unsigned>(Attrib::InstanceMatrix);
  constexpr int columnCount = 4;
  constexpr int columnComponents = 4;
  const auto stride = static_cast<int>(sizeof(glm::mat4));

  vao_.bindVertexBuffer(bindingIndex, instanceMatrixBuffer_->id(), 0, stride);
  for (int column = 0; column < columnCount; column++) {
    vao_.linkAttribFloat(bindingIndex + column, bindingIndex, columnComponents, false,
                         static_cast<unsigned>(sizeof(glm::vec4) * column));
  }
  vao_.setAttribDivisor(bindingIndex, 1);
}

void Geometry::setInstanceColors(std::span<const glm::vec4> colors) {
  if (!instanceColorBuffer_) {
    instanceColorBuffer_ = std::make_unique<Buffer>(colors, kDynamicInstance);
  } else {
    instanceColorBuffer_->setData(colors, kDynamicInstance);
  }

  constexpr auto bindingIndex = static_cast<unsigned>(Attrib::InstanceColor);
  vao_.bindVertexBuffer(bindingIndex, instanceColorBuffer_->id(), 0,
                        static_cast<int>(sizeof(glm::vec4)));
  vao_.linkAttribFloat(bindingIndex, bindingIndex, 4);
  vao_.setAttribDivisor(bindingIndex, 1);
}

void Geometry::setPrimitive(PrimitiveMode mode) {
  primitive_ = mode;
  spdlog::trace("Geometry({}) setPrimitive {}", uuidString(), static_cast<int>(mode));
}

void Geometry::setPatchType(PatchType type) {
  patchVertices_ = static_cast<int>(type);
  primitive_ = PrimitiveMode::Patches;
  spdlog::trace("Geometry({}) setPatchType {}", uuidString(), patchVertices_);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Geometry::setDrawRange(int start, int count) {
  drawRange_.start = std::max(0, start);
  drawRange_.count = std::max(0, count);
  spdlog::trace("Geometry({}) setDrawRange start={} count={}", uuidString(), drawRange_.start,
                drawRange_.count);
}

void Geometry::clearDrawRange() {
  drawRange_.start = 0;
  drawRange_.count = isIndexed_ ? indexCount_ : vertexCount_;
  spdlog::trace("Geometry({}) clearDrawRange -> start={} count={}", uuidString(), drawRange_.start,
                drawRange_.count);
}

PrimitiveMode Geometry::primitive() const {
  return primitive_;
}

int Geometry::patchVertices() const {
  return patchVertices_;
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

std::span<const float> Geometry::positions() const {
  return positions_;
}

std::span<const float> Geometry::uvs() const {
  return uvs_;
}

std::span<const float> Geometry::normals() const {
  return normals_;
}

std::span<const unsigned> Geometry::indices() const {
  return indices_;
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
