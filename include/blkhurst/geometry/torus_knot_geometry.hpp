// NOLINTBEGIN(readability-identifier-length)
#pragma once
#include <algorithm>
#include <blkhurst/geometry/geometry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <vector>

namespace blkhurst {

struct TorusKnotGeometryDesc {
  float radius = 1.0F;
  float tube = 0.4F;
  int tubularSegments = 64;
  int radialSegments = 8;
  int p = 2;
  int q = 3;
};

struct TorusKnotGeometry {
  static std::shared_ptr<Geometry> create(const TorusKnotGeometryDesc& desc = {}) {
    return Geometry::from(buildTorusKnot(desc));
  }

  static MeshData buildTorusKnot(TorusKnotGeometryDesc desc) {
    using glm::vec2;
    using glm::vec3;

    desc.tubularSegments = std::max(3, desc.tubularSegments);
    desc.radialSegments = std::max(3, desc.radialSegments);

    MeshData out;

    vec3 P1;
    vec3 P2;
    vec3 B;
    vec3 T;
    vec3 N;
    vec3 vertex;
    vec3 normal;

    // generate vertices, normals, uvs
    for (int i = 0; i <= desc.tubularSegments; ++i) {
      // Radian "u" is used to calculate position on the torus curve of the current tubular segment
      const float u = static_cast<float>(i) / static_cast<float>(desc.tubularSegments) *
                      static_cast<float>(desc.p) * glm::two_pi<float>();

      // now we calculate two points. P1 is our current position on the curve, P2 is a little
      // farther ahead. these points are used to create a special "coordinate space", which is
      // necessary to calculate the correct vertex positions
      calculatePositionOnCurve(u, desc.p, desc.q, desc.radius, P1);
      calculatePositionOnCurve(u + 0.01F, desc.p, desc.q, desc.radius, P2);

      // calculate orthonormal basis
      T = P2 - P1;
      N = P2 + P1;
      B = glm::cross(T, N);
      N = glm::cross(B, T);

      // normalize B, N. T can be ignored, we don't use it
      B = glm::normalize(B);
      N = glm::normalize(N);

      for (int j = 0; j <= desc.radialSegments; ++j) {
        // now calculate the vertices. they are nothing more than an extrusion of the torus curve.
        // because we extrude a shape in the xy-plane, there is no need to calculate a z-value.
        const float v =
            static_cast<float>(j) / static_cast<float>(desc.radialSegments) * glm::two_pi<float>();
        const float cx = -desc.tube * std::cos(v);
        const float cy = desc.tube * std::sin(v);

        // Position - First we orient the extrusion with our basis vectors, then we add it to the
        // current position on the curve
        vertex = P1 + (cx * N + cy * B);
        out.positions.insert(out.positions.end(), {vertex[0], vertex[1], vertex[2]});

        // Normal - (P1 is always the center/origin of the extrusion, thus we can use it to
        // calculate the normal)
        normal = glm::normalize(vertex - P1);
        out.normals.insert(out.normals.end(), {normal[0], normal[1], normal[2]});

        // uv
        out.uvs.insert(out.uvs.end(),
                       {static_cast<float>(i) / static_cast<float>(desc.tubularSegments),
                        static_cast<float>(j) / static_cast<float>(desc.radialSegments)});
      }
    }

    // indices
    for (int j = 1; j <= desc.tubularSegments; ++j) {
      for (int i = 1; i <= desc.radialSegments; ++i) {
        const unsigned a = (desc.radialSegments + 1) * (j - 1) + (i - 1);
        const unsigned b = (desc.radialSegments + 1) * j + (i - 1);
        const unsigned c = (desc.radialSegments + 1) * j + i;
        const unsigned d = (desc.radialSegments + 1) * (j - 1) + i;

        out.indices.insert(out.indices.end(), {a, b, d});
        out.indices.insert(out.indices.end(), {b, c, d});
      }
    }

    return out;
  }

private:
  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  static void calculatePositionOnCurve(float u, int p, int q, float radius, glm::vec3& position) {
    const float cu = std::cos(u);
    const float su = std::sin(u);
    const float quOverP = static_cast<float>(q) / static_cast<float>(p) * u;
    const float cs = std::cos(quOverP);

    position[0] = radius * (2.0F + cs) * 0.5F * cu;
    position[1] = radius * (2.0F + cs) * 0.5F * su;
    position[2] = radius * std::sin(quOverP) * 0.5F;
  }
};

} // namespace blkhurst
// NOLINTEND(readability-identifier-length)
