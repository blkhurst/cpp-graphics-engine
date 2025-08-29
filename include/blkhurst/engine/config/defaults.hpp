#pragma once

#include <blkhurst/engine/config/types.hpp>

namespace blkhurst::defaults {

// OpenGL
inline constexpr int GL_VERSION_MAJOR = 4;
inline constexpr int GL_VERSION_MINOR = 5;
inline constexpr GLVersion openGlVersion{GL_VERSION_MAJOR, GL_VERSION_MINOR};

// Logger
inline constexpr blkhurst::LogLevel logLevel = blkhurst::LogLevel::debug;

// Window
inline constexpr const char* windowTitle = "Blkhurst";
inline constexpr int windowPosX = 100;
inline constexpr int windowPosY = 100;
inline constexpr int windowWidth = 1600;
inline constexpr int windowHeight = 800;
inline constexpr int msaaSamples = 4;
inline constexpr bool vSync = true;
inline constexpr RGBA clearColor{0.07F, 0.13F, 0.17F, 1.0F}; // NOLINT(readability-magic-numbers)

} // namespace blkhurst::defaults
