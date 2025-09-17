#pragma once
#include <cstdint>
#include <vector>

namespace blkhurst {

struct MeshData {
  std::vector<float> positions;  // 3
  std::vector<uint32_t> indices; // 1
  std::vector<float> uvs;        // 2
  std::vector<float> normals;    // 3
  std::vector<float> tangents;   // 3
};

} // namespace blkhurst
