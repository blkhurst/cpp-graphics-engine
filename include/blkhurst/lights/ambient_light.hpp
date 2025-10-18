#pragma once
#include <blkhurst/lights/light.hpp>

namespace blkhurst {

class AmbientLight : public Light {
public:
  AmbientLight()
      : Light(LightType::Ambient) {
  }
};

} // namespace blkhurst
