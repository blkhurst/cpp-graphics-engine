#pragma once

#include <blkhurst/engine/root_state.hpp>
#include <string>

namespace blkhurst {

class UiEntry {
public:
  UiEntry() = default;
  virtual ~UiEntry() = default;

  UiEntry(const UiEntry&) = delete;
  UiEntry& operator=(const UiEntry&) = delete;
  UiEntry(UiEntry&&) = delete;
  UiEntry& operator=(UiEntry&&) = delete;

  void setTitle(std::string title) {
    title_ = std::move(title);
  }

  [[nodiscard]] const std::string& title() const {
    return title_;
  }

  virtual void onDraw(const RootState& /*state*/) {
  }

private:
  std::string title_;
};

} // namespace blkhurst
