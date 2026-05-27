#pragma once

#include <algorithm>
#include <array>
#include <imgui.h>
#include <span>

namespace blkhurst {

class PerformanceGraph {
public:
  struct Sample {
    float fps = 0.0F;
    float cpuMs = 0.0F;
    float gpuMs = 0.0F;
  };

  void pushSample(const Sample& sample) {
    const auto historyIndex = static_cast<std::size_t>(historyIndex_);
    fpsHistory_.at(historyIndex) = sample.fps;
    cpuMsHistory_.at(historyIndex) = sample.cpuMs;
    gpuMsHistory_.at(historyIndex) = sample.gpuMs;
    historyIndex_ = (historyIndex_ + 1) % kHistorySize;
    historyCount_ = std::min(historyCount_ + 1, kHistorySize);
  }

  void draw(float contentScale) const {
    const ImVec2 graphSize{kGraphWidth * contentScale, kGraphHeight * contentScale};
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(origin, {origin.x + graphSize.x, origin.y + graphSize.y},
                            kBackgroundColor, kRounding);
    drawList->AddRect(origin, {origin.x + graphSize.x, origin.y + graphSize.y}, kBorderColor,
                      kRounding);

    const float plotTop = origin.y + kPlotPadding;
    const float plotBottom = origin.y + graphSize.y - kPlotPadding;
    const float plotHeight = plotBottom - plotTop;

    drawSeries({.values = fpsHistory_, .maxValue = kFpsMax, .color = kFpsColor}, origin, graphSize,
               plotBottom, plotHeight, drawList);
    drawSeries({.values = cpuMsHistory_, .maxValue = kMsMax, .color = kCpuColor}, origin, graphSize,
               plotBottom, plotHeight, drawList);
    drawSeries({.values = gpuMsHistory_, .maxValue = kMsMax, .color = kGpuColor}, origin, graphSize,
               plotBottom, plotHeight, drawList);

    ImGui::Dummy(graphSize);

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::TextColored(ImColor(kFpsColor), "FPS");
    ImGui::TextColored(ImColor(kCpuColor), "CPU");
    ImGui::TextColored(ImColor(kGpuColor), "GPU");
    ImGui::EndGroup();
  }

private:
  struct Series {
    std::span<const float> values;
    float maxValue = 1.0F;
    ImU32 color = 0;
  };

  static constexpr int kHistorySize = 120;
  static constexpr float kGraphWidth = 178.0F;
  static constexpr float kGraphHeight = 52.0F;
  static constexpr float kFpsMax = 144.0F;
  static constexpr float kMsMax = 33.3F;
  static constexpr ImU32 kBackgroundColor = IM_COL32(24, 24, 24, 255);
  static constexpr ImU32 kBorderColor = IM_COL32(70, 70, 70, 255);
  static constexpr ImU32 kFpsColor = IM_COL32(92, 214, 128, 255);
  static constexpr ImU32 kCpuColor = IM_COL32(255, 193, 84, 255);
  static constexpr ImU32 kGpuColor = IM_COL32(104, 174, 255, 255);
  static constexpr float kRounding = 5.0F;
  static constexpr float kPlotPadding = 5.0F;
  static constexpr float kLineThickness = 1.5F;

  std::array<float, kHistorySize> fpsHistory_{};
  std::array<float, kHistorySize> cpuMsHistory_{};
  std::array<float, kHistorySize> gpuMsHistory_{};
  int historyIndex_ = 0;
  int historyCount_ = 0;

  void drawSeries(const Series& series, const ImVec2& origin, const ImVec2& graphSize,
                  float plotBottom, float plotHeight, ImDrawList* drawList) const {
    if (historyCount_ < 2) {
      return;
    }

    const int start = (historyIndex_ - historyCount_ + kHistorySize) % kHistorySize;
    ImVec2 previous{};
    for (int sampleIndex = 0; sampleIndex < historyCount_; sampleIndex++) {
      const int valueIndex = (start + sampleIndex) % kHistorySize;
      const float positionX =
          origin.x +
          (static_cast<float>(sampleIndex) / static_cast<float>(historyCount_ - 1)) * graphSize.x;

      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
      const float normalized = std::clamp(
          series.values[static_cast<std::size_t>(valueIndex)] / series.maxValue, 0.0F, 1.0F);
      const float positionY = plotBottom - normalized * plotHeight;

      const ImVec2 current{positionX, positionY};
      if (sampleIndex > 0) {
        drawList->AddLine(previous, current, series.color, kLineThickness);
      }
      previous = current;
    }
  }
};

} // namespace blkhurst
