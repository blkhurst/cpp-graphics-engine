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
struct TorusGeometryDesc {
  float radius = 1.0F;              // from center to center of tube
  float tube = 0.4F;                // radius of the tube
  int radialSegments = 12;          // segments around cross-section
  int tubularSegments = 48;         // segments around center ring
  float arc = glm::two_pi<float>(); // central angle
};
// NOLINTEND(readability-magic-numbers)

struct TorusGeometry {
  static std::shared_ptr<Geometry> create(const TorusGeometryDesc& desc = {}) {
    return Geometry::from(buildTorus(desc));
  }

  static MeshData buildTorus(TorusGeometryDesc desc) {
    using glm::vec3;

    desc.radialSegments = std::max(3, desc.radialSegments);
    desc.tubularSegments = std::max(3, desc.tubularSegments);

    MeshData out;

    // generate vertices, normals and uvs
    for (int j = 0; j <= desc.radialSegments; ++j) {
      for (int i = 0; i <= desc.tubularSegments; ++i) {
        const float u =
            (static_cast<float>(i) / static_cast<float>(desc.tubularSegments)) * desc.arc;
        const float v = (static_cast<float>(j) / static_cast<float>(desc.radialSegments)) *
                        glm::two_pi<float>();

        // vertex
        glm::vec3 vertex{
            (desc.radius + desc.tube * std::cos(v)) * std::cos(u),
            (desc.radius + desc.tube * std::cos(v)) * std::sin(u),
            desc.tube * std::sin(v),
        };
        out.positions.insert(out.positions.end(), {vertex[0], vertex[1], vertex[2]});

        // normal
        glm::vec3 center{desc.radius * std::cos(u), desc.radius * std::sin(u), 0.0F};
        glm::vec3 normal = glm::normalize(vertex - center);
        out.normals.insert(out.normals.end(), {normal[0], normal[1], normal[2]});

        // uv
        out.uvs.insert(out.uvs.end(),
                       {static_cast<float>(i) / static_cast<float>(desc.tubularSegments),
                        static_cast<float>(j) / static_cast<float>(desc.radialSegments)});
      }
    }

    // indices
    for (int j = 1; j <= desc.radialSegments; ++j) {
      for (int i = 1; i <= desc.tubularSegments; ++i) {
        const unsigned a = (desc.tubularSegments + 1) * j + i - 1;
        const unsigned b = (desc.tubularSegments + 1) * (j - 1) + i - 1;
        const unsigned c = (desc.tubularSegments + 1) * (j - 1) + i;
        const unsigned d = (desc.tubularSegments + 1) * j + i;

        out.indices.insert(out.indices.end(), {a, b, d});
        out.indices.insert(out.indices.end(), {b, c, d});
      }
    }

    return out;
  }
};

} // namespace blkhurst

// NOLINTEND(readability-identifier-length)
