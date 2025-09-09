#pragma once

#include "blkhurst/input/input_keys.hpp"
#include <blkhurst/events/event_bus.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace blkhurst {

enum class CursorMode { Normal, Hidden, Locked };

class Input {
public:
  Input(const EventBus& events);
  ~Input() = default;

  Input(const Input&) = delete;
  Input& operator=(const Input&) = delete;
  Input(Input&&) = delete;
  Input& operator=(Input&&) = delete;

  void beginFrame();
  void endFrame();

  [[nodiscard]] bool keyDown(Key key) const;
  [[nodiscard]] bool keyPressed(Key key) const;
  [[nodiscard]] bool keyReleased(Key key) const;

  [[nodiscard]] bool mouseDown(MouseButton button) const;
  [[nodiscard]] bool mousePressed(MouseButton button) const;
  [[nodiscard]] bool mouseReleased(MouseButton button) const;

  [[nodiscard]] glm::vec2 mousePosition() const;
  [[nodiscard]] glm::vec2 mouseDelta() const;
  [[nodiscard]] glm::vec2 scrollDelta() const;

  [[nodiscard]] glm::vec2 windowSize() const;
  [[nodiscard]] glm::vec2 framebufferSize() const;
  [[nodiscard]] glm::vec2 contentScale() const;

  [[nodiscard]] bool hasFocus() const;

  // Event forwarding (GLFW)
  void pushKey(int key, bool down);
  void pushMouseButton(int button, bool down);
  void pushMouseMove(double posX, double posY);
  void pushScroll(double deltaX, double deltaY);
  void setFocused(bool focused);

  void pushWindowSize(int width, int height);
  void pushFramebufferSize(int width, int height);
  void pushContentScale(float scaleX, float scaleY);

  void setCursorMode(CursorMode mode);
  void setCursorModeSetter(std::function<void(CursorMode)> setter);

private:
  const EventBus& events_;
  static constexpr int kMaxKeys = 512;
  static constexpr int kMaxMouseButtons = 8;

  std::vector<bool> keyDown_{std::vector<bool>(kMaxKeys, false)};
  std::vector<bool> keyPressed_{std::vector<bool>(kMaxKeys, false)};
  std::vector<bool> keyReleased_{std::vector<bool>(kMaxKeys, false)};
  std::vector<bool> keyPrev_{std::vector<bool>(kMaxKeys, false)};

  std::vector<bool> mouseDown_{std::vector<bool>(kMaxMouseButtons, false)};
  std::vector<bool> mousePressed_{std::vector<bool>(kMaxMouseButtons, false)};
  std::vector<bool> mouseReleased_{std::vector<bool>(kMaxMouseButtons, false)};
  std::vector<bool> mousePrev_{std::vector<bool>(kMaxMouseButtons, false)};

  bool mouseHasPrev_ = false;
  double mouseX_ = 0.0;
  double mouseY_ = 0.0;
  double mousePrevX_ = 0.0;
  double mousePrevY_ = 0.0;
  glm::vec2 mouseDelta_{0.0F, 0.0F};
  glm::vec2 scrollDelta_{0.0F, 0.0F};

  bool focused_ = true;
  int windowWidth_ = 0;
  int windowHeight_ = 0;
  int framebufferWidth_ = 0;
  int framebufferHeight_ = 0;
  float contentScaleX_ = 1.0F;
  float contentScaleY_ = 1.0F;

  std::function<void(CursorMode)> cursorModeSetter_;
};

} // namespace blkhurst
