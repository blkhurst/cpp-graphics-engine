#pragma once

#include <blkhurst/engine/root_state.hpp>

namespace blkhurst {

class Controller {
public:
  Controller() = default;
  virtual ~Controller() = default;

  Controller(const Controller&) = delete;
  Controller& operator=(const Controller&) = delete;
  Controller(Controller&&) = delete;
  Controller& operator=(Controller&&) = delete;

  virtual void update(const RootState& state) = 0;
};

} // namespace blkhurst
