#pragma once
#include <algorithm>
#include <blkhurst/geometry/geometry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <memory>
#include <vector>

namespace blkhurst {

struct CapsuleGeometryDesc {
  constexpr static int kRadialSegments = 8;

  float radius = 1.0F;                  // Radius of the capsule.
  float height = 1.0F;                  // Height of the middle section.
  int capSegments = 4;                  // Number of curve segments used to build each cap (>=1).
  int radialSegments = kRadialSegments; // Number of segmented faces around the circumference (>=3).
  int heightSegments = 1;               // Number of rows along the middle section (>=1).
};

struct CapsuleGeometry {
  static std::shared_ptr<Geometry> create(const CapsuleGeometryDesc& desc = {}) {
    return Geometry::from(buildCapsule(desc));
  }

  static MeshData buildCapsule(const CapsuleGeometryDesc& inDesc) {
    using glm::pi;
    using glm::vec2;
    using glm::vec3;

    // Ensure Valid Parameters
    CapsuleGeometryDesc desc = inDesc;
    desc.height = std::max(0.0F, desc.height);
    desc.capSegments = std::max(1, desc.capSegments);
    desc.radialSegments = std::max(3, desc.radialSegments);
    desc.heightSegments = std::max(1, desc.heightSegments);

    const float halfHeight = desc.height * 0.5F;
    const float capArcLength = (pi<float>() * 0.5F) * desc.radius;
    const float cylinderPartLength = desc.height;
    const float totalArcLength = 2.0F * capArcLength + cylinderPartLength;

    const int numVerticalSegments = desc.capSegments * 2 + desc.heightSegments;
    const int verticesPerRow = desc.radialSegments + 1;

    MeshData out;

    for (int iy = 0; iy <= numVerticalSegments; ++iy) {
      float currentArcLength = 0.0F;
      float profileY = 0.0F;
      float profileRadius = 0.0F;
      float normalYComponent = 0.0F;

      if (iy <= desc.capSegments) {
        // bottom cap
        const float segmentProgress = static_cast<float>(iy) / static_cast<float>(desc.capSegments);
        const float angle = segmentProgress * (pi<float>() * 0.5F);
        profileY = -halfHeight - desc.radius * std::cos(angle);
        profileRadius = desc.radius * std::sin(angle);
        normalYComponent = -desc.radius * std::cos(angle);
        currentArcLength = segmentProgress * capArcLength;
      } else if (iy <= desc.capSegments + desc.heightSegments) {
        // middle section
        const float segmentProgress =
            static_cast<float>(iy - desc.capSegments) / static_cast<float>(desc.heightSegments);
        profileY = -halfHeight + segmentProgress * desc.height;
        profileRadius = desc.radius;
        normalYComponent = 0.0F;
        currentArcLength = capArcLength + segmentProgress * cylinderPartLength;
      } else {
        // top cap
        const float segmentProgress =
            static_cast<float>(iy - desc.capSegments - desc.heightSegments) /
            static_cast<float>(desc.capSegments);
        const float angle = segmentProgress * (pi<float>() * 0.5F);
        profileY = halfHeight + desc.radius * std::sin(angle);
        profileRadius = desc.radius * std::cos(angle);
        normalYComponent = desc.radius * std::sin(angle);
        currentArcLength = capArcLength + cylinderPartLength + segmentProgress * capArcLength;
      }

      const float vCoord = std::clamp(currentArcLength / totalArcLength, 0.0F, 1.0F);

      // special case for the poles
      float uOffset = 0.0F;
      const float UvPoleOffset = 0.5F;
      if (iy == 0) {
        uOffset = UvPoleOffset / static_cast<float>(desc.radialSegments);
      } else if (iy == numVerticalSegments) {
        uOffset = -UvPoleOffset / static_cast<float>(desc.radialSegments);
      }

      for (int ix = 0; ix <= desc.radialSegments; ++ix) {
        const float uCoord = static_cast<float>(ix) / static_cast<float>(desc.radialSegments);
        const float theta = uCoord * pi<float>() * 2.0F;
        const float sinTheta = std::sin(theta);
        const float cosTheta = std::cos(theta);

        // vertex
        const vec3 vertex{
            -profileRadius * cosTheta,
            profileY,
            profileRadius * sinTheta,
        };
        out.positions.insert(out.positions.end(), {vertex[0], vertex[1], vertex[2]});

        // normal
        vec3 normal{
            -profileRadius * cosTheta,
            normalYComponent,
            profileRadius * sinTheta,
        };
        normal = glm::normalize(normal);
        out.normals.insert(out.normals.end(), {normal[0], normal[1], normal[2]});

        // uv
        out.uvs.push_back(uCoord + uOffset);
        out.uvs.push_back(vCoord);
      }

      if (iy > 0) {
        const int prevIndexRow = (iy - 1) * verticesPerRow;
        for (int ix = 0; ix < desc.radialSegments; ++ix) {
          const unsigned idxTopLeft = prevIndexRow + ix;
          const unsigned idxTopRight = prevIndexRow + ix + 1;
          const unsigned idxBottomLeft = iy * verticesPerRow + ix;
          const unsigned idxBottomRight = iy * verticesPerRow + ix + 1;

          out.indices.insert(out.indices.end(), {idxTopLeft, idxTopRight, idxBottomLeft});
          out.indices.insert(out.indices.end(), {idxTopRight, idxBottomRight, idxBottomLeft});
        }
      }
    }

    return out;
  }
};

} // namespace blkhurst
