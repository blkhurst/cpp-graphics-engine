#pragma once

#include <blkhurst/engine/config/types.hpp>

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
inline constexpr int posX = 100;
inline constexpr int posY = 100;
inline constexpr int width = 1600;
inline constexpr int height = 800;
inline constexpr int msaaSamples = 4;
inline constexpr bool vSync = true;
inline constexpr RGBA clearColor{0.07F, 0.13F, 0.17F, 1.0F};
} // namespace window

} // namespace blkhurst::defaults
