#pragma once

namespace blkhurst {

class Renderer;
class RenderTarget;

struct PassRenderContext {
  Renderer* renderer = nullptr;
  RenderTarget* writeBuffer = nullptr;
  RenderTarget* readBuffer = nullptr;
  bool renderToScreen = false;
};

class Pass {
public:
  Pass() = default;
  virtual ~Pass() = default;

  Pass(const Pass&) = delete;
  Pass(Pass&&) = delete;
  Pass& operator=(const Pass&) = delete;
  Pass& operator=(Pass&&) = delete;

  virtual void setSize(int width, int height);
  virtual void render(const PassRenderContext& context) = 0;

  [[nodiscard]] bool needsSwap() const;
  [[nodiscard]] bool isEnabled() const;
  void setEnabled(bool enabled);
  void setNeedsSwap(bool needsSwap);

private:
  bool enabled_ = true;
  bool needsSwap_ = true;
};

} // namespace blkhurst
