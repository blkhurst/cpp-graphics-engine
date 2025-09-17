// NOLINTBEGIN(readability-identifier-length)
#pragma once
#include <blkhurst/geometry/geometry.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace blkhurst {

struct BilinearQuadGeometryDesc {
  constexpr static int kDefaultWidthSegments = 10;
  constexpr static int kDefaultHeightSegments = 10;

  glm::vec3 v0 = {-1.0, -1.0, 1.0};
  glm::vec3 v1 = {1.0, -1.0, 0.0};
  glm::vec3 v2 = {1.0, 1.0, 1.0};
  glm::vec3 v3 = {-1.0, 1.0, 0.0};
  int widthSegments = kDefaultWidthSegments;
  int heightSegments = kDefaultHeightSegments;
};

struct BilinearQuadGeometry {
  static std::shared_ptr<Geometry> create(const BilinearQuadGeometryDesc& desc = {}) {

    return Geometry::from(buildBilinearQuad(desc));
  }

  // Non Planar Quadrilateral Mesh Using Bilinear Interpolation
  static MeshData buildBilinearQuad(const BilinearQuadGeometryDesc& desc) {
    using namespace glm;

    MeshData out;

    //
    const int segmentsX = std::max(1, desc.widthSegments);
    const int segmentsY = std::max(1, desc.heightSegments);
    const int stride = segmentsX + 1;

    /* Plane Corners (Passed As Input)
        v3 --- v2
        |       |
        |       |
        v0 --- v1
    */
    vec3 v0 = desc.v0;
    vec3 v1 = desc.v1;
    vec3 v2 = desc.v2;
    vec3 v3 = desc.v3;

    // Get Left Edge Vector & Right Edge Vector
    // Divide By Segments To Get Step For Bilinear Interpolation.
    //- leftEdgeStep: 0 -> 3 division step
    //- rightEdgeStep: 1 -> 2 division step
    vec3 leftEdgeStep = (v3 - v0) / static_cast<float>(segmentsY);
    vec3 rightEdgeStep = (v2 - v1) / static_cast<float>(segmentsY);

    // Generate A Grid Of Vertex Points
    for (int row = 0; row <= segmentsY; ++row) {
      auto rowFloat = static_cast<float>(row);

      // Calculate Left Edge Position & Right Edge Position
      vec3 leftEdgePoint = v0 + (leftEdgeStep * rowFloat);   // Get height up left edge
      vec3 rightEdgePoint = v1 + (rightEdgeStep * rowFloat); // Get height up right edge

      // LeftPoint -> RightPoint step for this row
      vec3 crossStep = (rightEdgePoint - leftEdgePoint) / static_cast<float>(segmentsX);

      for (int col = 0; col <= segmentsX; ++col) {
        auto colFloat = static_cast<float>(col);

        // Position
        vec3 currentPos = leftEdgePoint + (crossStep * colFloat); // Interpolate between edges
        out.positions.insert(out.positions.end(), {currentPos[0], currentPos[1], currentPos[2]});

        // Indices
        if (row < segmentsY && col < segmentsX) {
          int index_in_grid = row * stride + col;

          out.indices.push_back(index_in_grid);              // 0    3---2   3---2
          out.indices.push_back(index_in_grid + stride + 1); // 2    | '     |   |
          out.indices.push_back(index_in_grid + stride);     // 3    0       0---1

          out.indices.push_back(index_in_grid);              // 0        2   3---2
          out.indices.push_back(index_in_grid + 1);          // 1      ' |   |   |
          out.indices.push_back(index_in_grid + stride + 1); // 2    0---1   0---1
        }

        // Texture UVs
        out.uvs.push_back(colFloat / static_cast<float>(segmentsX)); // U
        out.uvs.push_back(rowFloat / static_cast<float>(segmentsY)); // V

        // Normals
        vec3 nextLeftEdge = leftEdgePoint + leftEdgeStep;    // Move up left edge
        vec3 nextRightEdge = rightEdgePoint + rightEdgeStep; // Move up right edge
        vec3 nextCrossStep = (nextRightEdge - nextLeftEdge) / static_cast<float>(segmentsX);
        vec3 nextRowPos = nextLeftEdge + (nextCrossStep * colFloat); // Move along cross vec

        vec3 tangent = normalize(crossStep);                 // X ->
        vec3 bitangent = normalize(nextRowPos - currentPos); // Y ^
        vec3 normal = normalize(cross(tangent, bitangent));

        out.tangents.insert(out.tangents.end(), {tangent[0], tangent[1], tangent[2]});
        out.normals.insert(out.normals.end(), {normal[0], normal[1], normal[2]});
      }
    }

    return out;
  }
};

} // namespace blkhurst

// NOLINTEND(readability-identifier-length)
