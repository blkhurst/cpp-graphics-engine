// NOLINTBEGIN(readability-identifier-length)
#pragma once
#include <algorithm>
#include <blkhurst/geometry/geometry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace blkhurst {

// NOLINTBEGIN(readability-magic-numbers)
struct SphereGeometryDesc {
  float radius = 1.0F;                    // sphere radius
  int widthSegments = 32;                 // horizontal segments (>=3)
  int heightSegments = 16;                // vertical segments (>=2)
  float phiStart = 0.0F;                  // horizontal start angle
  float phiLength = glm::two_pi<float>(); // horizontal sweep
  float thetaStart = 0.0F;                // vertical start angle
  float thetaLength = glm::pi<float>();   // vertical sweep
};
// NOLINTEND(readability-magic-numbers)

struct SphereGeometry {
  static std::shared_ptr<Geometry> create(const SphereGeometryDesc& desc = {}) {
    return Geometry::from(buildSphere(desc));
  }

  static MeshData buildSphere(SphereGeometryDesc desc) {
    using glm::vec2;
    using glm::vec3;

    desc.widthSegments = std::max(3, desc.widthSegments);
    desc.heightSegments = std::max(2, desc.heightSegments);

    const float thetaEnd = std::min(desc.thetaStart + desc.thetaLength, glm::pi<float>());

    MeshData out;

    int runningIndex = 0;
    std::vector<std::vector<int>> grid;

    // generate vertices, normals and uvs
    for (int iy = 0; iy <= desc.heightSegments; ++iy) {
      std::vector<int> verticesRow;
      const float v = static_cast<float>(iy) / static_cast<float>(desc.heightSegments);

      // special case for the poles
      float uOffset = 0.0F;
      const float kUvPoleOffset = 0.5F;
      if (iy == 0 && desc.thetaStart == 0.0F) {
        uOffset = kUvPoleOffset / static_cast<float>(desc.widthSegments);
      } else if (iy == desc.heightSegments && thetaEnd == glm::pi<float>()) {
        uOffset = -kUvPoleOffset / static_cast<float>(desc.widthSegments);
      }

      for (int ix = 0; ix <= desc.widthSegments; ++ix) {
        const float u = static_cast<float>(ix) / static_cast<float>(desc.widthSegments);

        // vertex
        const float phi = desc.phiStart + u * desc.phiLength;
        const float theta = desc.thetaStart + v * desc.thetaLength;

        glm::vec3 vertex{
            -desc.radius * std::cos(phi) * std::sin(theta),
            desc.radius * std::cos(theta),
            desc.radius * std::sin(phi) * std::sin(theta),
        };
        out.positions.insert(out.positions.end(), {vertex[0], vertex[1], vertex[2]});

        // normal
        glm::vec3 normal = glm::normalize(vertex);
        out.normals.insert(out.normals.end(), {normal[0], normal[1], normal[2]});

        // uv
        out.uvs.insert(out.uvs.end(), {u + uOffset, 1.0F - v});

        verticesRow.push_back(runningIndex++);
      }

      grid.push_back(std::move(verticesRow));
    }

    // indices
    for (int iy = 0; iy < desc.heightSegments; ++iy) {
      for (int ix = 0; ix < desc.widthSegments; ++ix) {
        const unsigned a = grid[iy][ix + 1];
        const unsigned b = grid[iy][ix];
        const unsigned c = grid[iy + 1][ix];
        const unsigned d = grid[iy + 1][ix + 1];

        if (iy != 0 || desc.thetaStart > 0.0F) {
          out.indices.insert(out.indices.end(), {a, b, d});
        }
        if (iy != desc.heightSegments - 1 || thetaEnd < glm::pi<float>()) {
          out.indices.insert(out.indices.end(), {b, c, d});
        }
      }
    }

    return out;
  }
};

} // namespace blkhurst

// NOLINTEND(readability-identifier-length)
