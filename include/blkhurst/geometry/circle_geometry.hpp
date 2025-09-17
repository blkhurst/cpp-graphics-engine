// NOLINTBEGIN(readability-identifier-length)
#pragma once
#include <algorithm>
#include <blkhurst/geometry/geometry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <vector>

namespace blkhurst {

/**
 * A simple shape of Euclidean geometry. It is constructed from a
 * number of triangular segments that are oriented around a central point and
 * extend as far out as a given radius. It is built counter-clockwise from a
 * start angle and a given central angle. It can also be used to create
 * regular polygons, where the number of segments determines the number of
 * sides.
 */
// NOLINTBEGIN(readability-magic-numbers)
struct CircleGeometryDesc {
  float radius = 1.0F;                      // Radius of the circle.
  int segments = 32;                        // Number of segments (>=3).
  float thetaStart = 0.0F;                  // Start angle in radians.
  float thetaLength = glm::two_pi<float>(); // Central angle (default full circle).
};
// NOLINTEND(readability-magic-numbers)

struct CircleGeometry {
  static std::shared_ptr<Geometry> create(const CircleGeometryDesc& desc = {}) {
    return Geometry::from(buildCircle(desc));
  }

  static MeshData buildCircle(CircleGeometryDesc desc) {
    using glm::vec3;

    desc.segments = std::max(3, desc.segments);

    MeshData out;

    // center point
    out.positions.insert(out.positions.end(), {0.0F, 0.0F, 0.0F});
    out.normals.insert(out.normals.end(), {0.0F, 0.0F, 1.0F});
    out.tangents.insert(out.tangents.end(), {1.0F, 0.0F, 0.0F}); // arbitrary in-plane at center
    out.uvs.insert(out.uvs.end(), {0.5F, 0.5F});

    for (int s = 0, i = 3; s <= desc.segments; ++s, i += 3) {
      const float segment =
          desc.thetaStart +
          (static_cast<float>(s) / static_cast<float>(desc.segments)) * desc.thetaLength;

      // Vertex
      const float x = desc.radius * std::cos(segment);
      const float y = desc.radius * std::sin(segment);
      out.positions.insert(out.positions.end(), {x, y, 0.0F});

      // Normal
      out.normals.insert(out.normals.end(), {0.0F, 0.0F, 1.0F});

      // Uvs
      const float u = (x / desc.radius + 1.0F) * 0.5F;
      const float v = (y / desc.radius + 1.0F) * 0.5F;
      out.uvs.insert(out.uvs.end(), {u, v});
    }

    // indices
    for (unsigned i = 1; i <= desc.segments; ++i) {
      out.indices.insert(out.indices.end(), {i, i + 1, 0U});
    }

    return out;
  }
};

} // namespace blkhurst

// NOLINTEND(readability-identifier-length)
