#include "blkhurst/input/input_keys.hpp"
#include <blkhurst/events/events.hpp>
#include <blkhurst/input/input.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

Input::Input(const EventBus& events)
    : events_(events) {
}

void Input::beginFrame() {
  // Reset per-frame edges
  std::fill(keyPressed_.begin(), keyPressed_.end(), false);
  std::fill(keyReleased_.begin(), keyReleased_.end(), false);
  std::fill(mousePressed_.begin(), mousePressed_.end(), false);
  std::fill(mouseReleased_.begin(), mouseReleased_.end(), false);

  // Reset per-frame deltas
  mouseDelta_ = {0.0F, 0.0F};
  scrollDelta_ = {0.0F, 0.0F};
}

void Input::endFrame() {
  // Compute edges from prev -> current
  for (int i = 0; i < kMaxKeys; ++i) {
    const bool down = keyDown_[i];
    const bool prev = keyPrev_[i];
    keyPressed_[i] = down && !prev;
    keyReleased_[i] = !down && prev;
    keyPrev_[i] = down;
  }
  for (int button = 0; button < kMaxMouseButtons; ++button) {
    const bool down = mouseDown_[button];
    const bool prev = mousePrev_[button];
    mousePressed_[button] = down && !prev;
    mouseReleased_[button] = !down && prev;
    mousePrev_[button] = down;
  }
}

bool Input::keyDown(Key key) const {
  auto keyVal = static_cast<int>(key);
  if (keyVal < 0 || keyVal >= kMaxKeys) {
    return false;
  }
  return keyDown_[keyVal];
}

bool Input::keyPressed(Key key) const {
  auto keyVal = static_cast<int>(key);
  if (keyVal < 0 || keyVal >= kMaxKeys) {
    return false;
  }
  return keyPressed_[keyVal];
}

bool Input::keyReleased(Key key) const {
  auto keyVal = static_cast<int>(key);
  if (keyVal < 0 || keyVal >= kMaxKeys) {
    return false;
  }
  return keyReleased_[keyVal];
}

bool Input::mouseDown(MouseButton button) const {
  auto buttonVal = static_cast<int>(button);
  if (buttonVal < 0 || buttonVal >= kMaxMouseButtons) {
    return false;
  }
  return mouseDown_[buttonVal];
}

bool Input::mousePressed(MouseButton button) const {
  auto buttonVal = static_cast<int>(button);
  if (buttonVal < 0 || buttonVal >= kMaxMouseButtons) {
    return false;
  }
  return mousePressed_[buttonVal];
}

bool Input::mouseReleased(MouseButton button) const {
  auto buttonVal = static_cast<int>(button);
  if (buttonVal < 0 || buttonVal >= kMaxMouseButtons) {
    return false;
  }
  return mouseReleased_[buttonVal];
}

glm::vec2 Input::mousePosition() const {
  return {mouseX_, mouseY_};
}

glm::vec2 Input::mouseDelta() const {
  return mouseDelta_;
}

glm::vec2 Input::scrollDelta() const {
  return scrollDelta_;
}

glm::vec2 Input::windowSize() const {
  return {static_cast<float>(windowWidth_), static_cast<float>(windowHeight_)};
}

glm::vec2 Input::framebufferSize() const {
  return {static_cast<float>(framebufferWidth_), static_cast<float>(framebufferHeight_)};
}

glm::vec2 Input::contentScale() const {
  return {contentScaleX_, contentScaleY_};
}

bool Input::hasFocus() const {
  return focused_;
}

// GLFW
void Input::pushKey(int key, bool down) {
  if (key < 0 || key >= kMaxKeys) {
    return;
  }

  if (down && !keyDown_[key]) {
    keyPressed_[key] = true;
  } else if (!down && keyDown_[key]) {
    keyReleased_[key] = true;
  }
  keyDown_[key] = down;
}

void Input::pushMouseButton(int button, bool down) {
  if (button < 0 || button >= kMaxMouseButtons) {
    return;
  }

  if (down && !mouseDown_[button]) {
    mousePressed_[button] = true;
  } else if (!down && mouseDown_[button]) {
    mouseReleased_[button] = true;
  }
  mouseDown_[button] = down;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Input::pushMouseMove(double posX, double posY) {
  mouseX_ = posX;
  mouseY_ = posY;

  if (!mouseHasPrev_) {
    mousePrevX_ = posX;
    mousePrevY_ = posY;
    mouseHasPrev_ = true;
  }

  mouseDelta_[0] += static_cast<float>(posX - mousePrevX_);
  mouseDelta_[1] += static_cast<float>(posY - mousePrevY_);
  mousePrevX_ = posX;
  mousePrevY_ = posY;
}

void Input::pushScroll(double deltaX, double deltaY) {
  scrollDelta_[0] += static_cast<float>(deltaX);
  scrollDelta_[1] += static_cast<float>(deltaY);
}

void Input::setFocused(bool focused) {
  focused_ = focused;
}

void Input::pushWindowSize(int width, int height) {
  windowWidth_ = std::max(0, width);
  windowHeight_ = std::max(0, height);

  // if (events_ != nullptr) {
  //   events_->emit<events::WindowResized>(windowWidth_, windowHeight_);
  // }
}

void Input::pushFramebufferSize(int width, int height) {
  framebufferWidth_ = std::max(0, width);
  framebufferHeight_ = std::max(0, height);
  spdlog::debug("Window framebuffer resized {}x{}", width, height);

  events_.emit<events::FramebufferResized>(framebufferWidth_, framebufferHeight_);
}

void Input::pushContentScale(float scaleX, float scaleY) {
  contentScaleX_ = std::max(0.0F, scaleX);
  contentScaleY_ = std::max(0.0F, scaleY);

  // if (events_ != nullptr) {
  //   events_->emit<events::ContentScaleChanged>(contentScaleX_, contentScaleY_);
  // }
}

void Input::setCursorMode(CursorMode mode) {
  if (cursorModeSetter_) {
    cursorModeSetter_(mode);
  } else {
    spdlog::warn("Input::setCursorMode ignored (no platform setter)");
  }
}

void Input::setCursorModeSetter(std::function<void(CursorMode)> setter) {
  cursorModeSetter_ = std::move(setter);
}

} // namespace blkhurst
