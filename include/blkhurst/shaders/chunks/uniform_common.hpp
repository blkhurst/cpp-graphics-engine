#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string uniforms_common = R"GLSL(

// FrameUniforms
uniform float uTime;
uniform float uDelta;
uniform vec2 uMouse;
uniform vec2 uResolution;
// pad0_ pad1_
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uCameraPos;
uniform bool uIsOrthographic;
//

// DrawUniforms
uniform mat4 uModel;

)GLSL";

} // namespace blkhurst::shaders
