// NOLINTBEGIN(readability-identifier-length)
#pragma once
#include <blkhurst/geometry/geometry.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace blkhurst {

struct PlaneGeometryDesc {
  float width = 1.0F;
  float height = 1.0F;
  int widthSegments = 1;
  int heightSegments = 1;
};

struct PlaneGeometry {
  static std::shared_ptr<Geometry> create(const PlaneGeometryDesc& desc = {}) {

    return Geometry::from(buildPlane(desc));
  }

  // Planar Mesh Using Linear Interpolation
  static MeshData buildPlane(const PlaneGeometryDesc& desc) {
    using namespace glm;

    MeshData out;

    //
    const int segmentsX = std::max(1, desc.widthSegments);
    const int segmentsY = std::max(1, desc.heightSegments);
    const float widthHalf = desc.width * 0.5F;
    const float heightHalf = desc.height * 0.5F;
    const int stride = segmentsX + 1;

    // Plane Corners
    vec3 v0 = vec3(-widthHalf, -heightHalf, 0.0F); // 0    3   2
    vec3 v1 = vec3(widthHalf, -heightHalf, 0.0F);  // 1    |
    vec3 v3 = vec3(-widthHalf, heightHalf, 0.0F);  // 3    0---1

    // Row & Column Step Calculated By Number Of Segments
    vec3 rowStep = (v3 - v0) / static_cast<float>(segmentsY); // 0 -> 3
    vec3 colStep = (v1 - v0) / static_cast<float>(segmentsX); // 0 -> 1

    // Generate A Grid Of Vertex Points
    for (int row = 0; row <= segmentsY; ++row) {
      for (int col = 0; col <= segmentsX; ++col) {
        auto rowFloat = static_cast<float>(row);
        auto colFloat = static_cast<float>(col);

        // Position
        vec3 currentVec = v0 + (colStep * colFloat) + (rowStep * rowFloat);
        out.positions.insert(out.positions.end(), {currentVec[0], currentVec[1], currentVec[2]});

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

        // Normals (constant planar)
        vec3 tangent = normalize(colStep);   // X ->
        vec3 bitangent = normalize(rowStep); // Y ^
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
