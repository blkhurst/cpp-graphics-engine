#pragma once
#include <blkhurst/objects/object3d.hpp>

namespace blkhurst {

class Group : public Object3D {
public:
  Group()
      : Object3D(NodeType::Group) {
  }
};

} // namespace blkhurst
