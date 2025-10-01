#pragma once
#include <string>

namespace blkhurst::shaders {

inline const std::string fullscreen_vert = R"GLSL(

#include "io_vertex"
#include "uniforms_common"

void main() {
  io_vertex(uModel, uView, uProjection); // Used for attributes + varyings
  gl_Position = vec4(aPosition, 1.0); // Overwrite with clip position
}

)GLSL";

} // namespace blkhurst::shaders
