#pragma once
#include <cstdint>

namespace blkhurst {

enum class CullFace : std::uint8_t { Back, Front, None };

struct PipelineDefaults {
  static constexpr bool depthTest = true;
  static constexpr bool depthWrite = true;
  static constexpr bool blend = false;
  static constexpr CullFace cull = CullFace::Back;
};

struct PipelineState {
  bool depthTest = PipelineDefaults::depthTest;
  bool depthWrite = PipelineDefaults::depthWrite;
  bool blend = PipelineDefaults::blend;
  CullFace cull = PipelineDefaults::cull;
};

} // namespace blkhurst
