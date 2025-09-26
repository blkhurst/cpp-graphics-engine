#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string io_fragment = R"GLSL(

// TODO: Extract when adding MRT (Multiple Render Target) support
layout(location = 0) out vec4 FragColor;

in vec2 vUv;
in vec4 vColor;
in vec3 vWorldNormal;
in vec3 vWorldPosition;
in vec3 vViewPosition;
in vec4 vInstanceColor;

)GLSL";

} // namespace blkhurst::shaders
