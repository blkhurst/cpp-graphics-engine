// NOLINTBEGIN(readability-identifier-length)
#pragma once
#include <algorithm>
#include <blkhurst/geometry/geometry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <vector>

namespace blkhurst {

// NOLINTBEGIN(readability-magic-numbers)
struct CylinderGeometryDesc {
  float radiusTop = 1.0F;
  float radiusBottom = 1.0F;
  float height = 1.0F;
  int radialSegments = 32;
  int heightSegments = 1;
  bool openEnded = false;
  float thetaStart = 0.0F;
  float thetaLength = glm::two_pi<float>();
};
// NOLINTEND(readability-magic-numbers)

struct CylinderGeometry {
  static std::shared_ptr<Geometry> create(const CylinderGeometryDesc& desc = {}) {
    return Geometry::from(buildCylinder(desc));
  }

  static MeshData buildCylinder(CylinderGeometryDesc desc) {
    using glm::vec2;
    using glm::vec3;

    // three.js floors these; we ensure minimums here
    desc.radialSegments = std::max(3, desc.radialSegments);
    desc.heightSegments = std::max(1, desc.heightSegments);

    MeshData out;

    int index = 0;                            // running vertex index
    std::vector<std::vector<int>> indexArray; // [y][x]
    const float halfHeight = desc.height * 0.5F;

    generateTorso(out, desc, index, indexArray, halfHeight);

    if (!desc.openEnded) {
      if (desc.radiusTop > 0.0F) {
        generateCap(out, desc, /*top=*/true, index, halfHeight);
      }
      if (desc.radiusBottom > 0.0F) {
        generateCap(out, desc, /*top=*/false, index, halfHeight);
      }
    }

    return out;
  }

private:
  // Push one vertex (position/normal/uv) and increment index
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  static inline void pushVertex(MeshData& out, const glm::vec3& p, const glm::vec3& n,
                                const glm::vec2& uv, int& index) {
    out.positions.insert(out.positions.end(), {p[0], p[1], p[2]});
    out.normals.insert(out.normals.end(), {n[0], n[1], n[2]});
    out.uvs.insert(out.uvs.end(), {uv[0], uv[1]});
    ++index;
  }

  static void generateTorso(MeshData& out, const CylinderGeometryDesc& d, int& index,
                            std::vector<std::vector<int>>& indexArray, float halfHeight) {
    using glm::vec2;
    using glm::vec3;

    // matches three.js: slope = (rb - rt) / height
    const float slope = (d.radiusBottom - d.radiusTop) / d.height;

    // vertices, normals, uvs
    for (int y = 0; y <= d.heightSegments; ++y) {
      std::vector<int> row;
      const float v = static_cast<float>(y) / static_cast<float>(d.heightSegments);
      const float radius = v * (d.radiusBottom - d.radiusTop) + d.radiusTop;

      for (int x = 0; x <= d.radialSegments; ++x) {
        const float u = static_cast<float>(x) / static_cast<float>(d.radialSegments);
        const float theta = u * d.thetaLength + d.thetaStart;

        const float s = std::sin(theta);
        const float c = std::cos(theta);

        glm::vec3 pos{radius * s, -v * d.height + halfHeight, radius * c};
        glm::vec3 nrm = glm::normalize(glm::vec3{s, slope, c});
        glm::vec2 uv{u, 1.0F - v};

        pushVertex(out, pos, nrm, uv, index);
        row.push_back(index - 1);
      }

      indexArray.push_back(std::move(row));
    }

    // indices (same pattern as three.js)
    for (int x = 0; x < d.radialSegments; ++x) {
      for (int y = 0; y < d.heightSegments; ++y) {
        const unsigned a = indexArray[y][x];
        const unsigned b = indexArray[y + 1][x];
        const unsigned c = indexArray[y + 1][x + 1];
        const unsigned e = indexArray[y][x + 1];

        if (d.radiusTop > 0.0F || y != 0) {
          out.indices.insert(out.indices.end(), {a, b, e});
        }
        if (d.radiusBottom > 0.0F || y != d.heightSegments - 1) {
          out.indices.insert(out.indices.end(), {b, c, e});
        }
      }
    }
  }

  static void generateCap(MeshData& out, const CylinderGeometryDesc& d, bool top, int& index,
                          float halfHeight) {
    using glm::vec2;
    using glm::vec3;

    const float radius = top ? d.radiusTop : d.radiusBottom;
    const float sign = top ? 1.0F : -1.0F;

    // save index of the first center vertex
    const int centerIndexStart = index;

    // we generate a center vertex per segment (like three.js)
    for (int x = 1; x <= d.radialSegments; ++x) {
      glm::vec3 pos{0.0F, halfHeight * sign, 0.0F};
      glm::vec3 nrm{0.0F, sign, 0.0F};
      glm::vec2 uv{0.5F, 0.5F};
      pushVertex(out, pos, nrm, uv, index);
    }

    // last center vertex index
    const int centerIndexEnd = index;

    // rim vertices
    for (int x = 0; x <= d.radialSegments; ++x) {
      const float u = static_cast<float>(x) / static_cast<float>(d.radialSegments);
      const float theta = u * d.thetaLength + d.thetaStart;

      const float c = std::cos(theta);
      const float s = std::sin(theta);

      glm::vec3 pos{radius * s, halfHeight * sign, radius * c};
      glm::vec3 nrm{0.0F, sign, 0.0F};
      glm::vec2 uv{(c * 0.5F) + 0.5F, (s * 0.5F * sign) + 0.5F};

      pushVertex(out, pos, nrm, uv, index);
    }

    // indices (fan per segment)
    for (int x = 0; x < d.radialSegments; ++x) {
      const unsigned c = centerIndexStart + x;
      const unsigned i = centerIndexEnd + x;

      if (top) {
        // face top
        out.indices.insert(out.indices.end(), {i, i + 1, c});
      } else {
        // face bottom
        out.indices.insert(out.indices.end(), {i + 1, i, c});
      }
    }
  }
};

} // namespace blkhurst
// NOLINTEND(readability-identifier-length)
