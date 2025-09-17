#pragma once
#include <blkhurst/geometry/geometry.hpp>
#include <glm/glm.hpp>
#include <memory>

namespace blkhurst {

struct BoxGeometryDesc {
  float width = 1.0F;
  float height = 1.0F;
  float depth = 1.0F;
  int widthSegments = 1;
  int heightSegments = 1;
  int depthSegments = 1;
};

struct BoxGeometry {
  static std::shared_ptr<Geometry> create(const BoxGeometryDesc& desc = {}) {
    return Geometry::from(buildBox(desc));
  }

  static MeshData buildBox(const BoxGeometryDesc& desc) {
    MeshData out;

    const int segW = std::max(1, desc.widthSegments);
    const int segH = std::max(1, desc.heightSegments);
    const int segD = std::max(1, desc.depthSegments);

    // +X (px)
    buildPlane(out, Axis::Z, Axis::Y, Axis::X, -1.0F, -1.0F, desc.depth, desc.height, desc.width,
               segD, segH);
    // -X (nx)
    buildPlane(out, Axis::Z, Axis::Y, Axis::X, 1.0F, -1.0F, desc.depth, desc.height, -desc.width,
               segD, segH);
    // +Y (py)
    buildPlane(out, Axis::X, Axis::Z, Axis::Y, 1.0F, 1.0F, desc.width, desc.depth, desc.height,
               segW, segD);
    // -Y (ny)
    buildPlane(out, Axis::X, Axis::Z, Axis::Y, 1.0F, -1.0F, desc.width, desc.depth, -desc.height,
               segW, segD);
    // +Z (pz)
    buildPlane(out, Axis::X, Axis::Y, Axis::Z, 1.0F, -1.0F, desc.width, desc.height, desc.depth,
               segW, segH);
    // -Z (nz)
    buildPlane(out, Axis::X, Axis::Y, Axis::Z, -1.0F, -1.0F, desc.width, desc.height, -desc.depth,
               segW, segH);

    return out;
  }

private:
  enum class Axis : int { X = 0, Y = 1, Z = 2 };

  static inline void setComp(glm::vec3& vec, Axis axis, float value) {
    vec[static_cast<int>(axis)] = value;
  }

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters, readability-identifier-length)
  static void buildPlane(MeshData& out, Axis u, Axis v, Axis w, float udir, float vdir, float width,
                         float height, float depth, int gridX, int gridY) {
    using glm::vec3;

    const float segW = width / static_cast<float>(gridX);
    const float segH = height / static_cast<float>(gridY);

    const float widthHalf = width * 0.5F;
    const float heightHalf = height * 0.5F;
    const float depthHalf = depth * 0.5F;

    const int gridX1 = gridX + 1;
    const int gridY1 = gridY + 1;

    const auto vertexStart = static_cast<uint32_t>(out.positions.size() / 3);

    // Vertices, normals, tangents, uvs
    for (int iy = 0; iy < gridY1; ++iy) {
      const float posY = static_cast<float>(iy) * segH - heightHalf;
      for (int ix = 0; ix < gridX1; ++ix) {
        const float posX = static_cast<float>(ix) * segW - widthHalf;

        vec3 pos(0.0F);
        setComp(pos, u, posX * udir);
        setComp(pos, v, posY * vdir);
        setComp(pos, w, depthHalf);

        out.positions.insert(out.positions.end(), {pos[0], pos[1], pos[2]});

        glm::vec3 normal(0.0F);
        setComp(normal, w, depth > 0.0F ? 1.0F : -1.0F);
        out.normals.insert(out.normals.end(), {normal[0], normal[1], normal[2]});

        // UVs (V flipped)
        out.uvs.push_back(static_cast<float>(ix) / static_cast<float>(gridX));
        out.uvs.push_back(1.0F - (static_cast<float>(iy) / static_cast<float>(gridY)));
      }
    }

    // Indices (two triangles per grid cell)
    for (int iy = 0; iy < gridY; ++iy) {
      for (int ix = 0; ix < gridX; ++ix) {
        const uint32_t topLeft = vertexStart + ix + gridX1 * (iy + 1);
        const uint32_t topRight = vertexStart + (ix + 1) + gridX1 * (iy + 1);
        const uint32_t bottomLeft = vertexStart + ix + gridX1 * iy;
        const uint32_t bottomRight = vertexStart + (ix + 1) + gridX1 * iy;

        // CCW when viewed from outside
        out.indices.insert(out.indices.end(),
                           {topLeft, topRight, bottomRight, topLeft, bottomRight, bottomLeft});
      }
    }
  }
};

} // namespace blkhurst
