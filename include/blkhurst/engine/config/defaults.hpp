#pragma once

#include <blkhurst/engine/config/types.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace blkhurst::defaults {

namespace opengl {
inline constexpr int major = 4;
inline constexpr int minor = 5;
inline constexpr GLVersion version{major, minor};
} // namespace opengl

namespace logger {
inline constexpr LogLevel logLevel = LogLevel::warn;
}

namespace window {
inline constexpr const char* title = "Blkhurst";
inline constexpr glm::ivec2 pos = {100, 100};
inline constexpr glm::ivec2 size = {1600, 800};
inline constexpr int msaaSamples = 4;
inline constexpr bool vSync = true;
inline constexpr glm::vec4 clearColor = {0.07F, 0.13F, 0.17F, 1.0F};
} // namespace window

namespace ui {
inline constexpr const char* title = "Blkhurst";

inline constexpr float fontSize = 14.0F;
inline constexpr const char* fontPath = "fonts/Inter.ttc";

inline constexpr float scale = 1.0F;
inline constexpr float minWindowWidth = 225.0F;

inline constexpr bool useDefaultStyle = true;
inline constexpr bool showStatsHeader = true;
inline constexpr bool showScenesHeader = true;
} // namespace ui

} // namespace blkhurst::defaults
