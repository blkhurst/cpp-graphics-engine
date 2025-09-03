#include "ui/ui_manager.hpp"
#include "blkhurst/util/assets.hpp"

#include <cstdio>
#include <glad/gl.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

namespace {
constexpr ImGuiWindowFlags kMainWindowFlags =
    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove;

constexpr float kWindowPosX = 10.0F;
constexpr float kWindowPosY = 10.0F;
} // namespace

namespace blkhurst {

UiManager::UiManager(UiConfig config, WindowManager& windowManager)
    : config_(std::move(config)),
      window_(windowManager) {
  spdlog::debug("UiManager initialising...");
  initialiseImGui(windowManager);
  spdlog::debug("UiManager initialised scale={:.2f}", contentScale_);
}

UiManager::~UiManager() {
  if (ImGui::GetCurrentContext() == nullptr) {
    return;
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  spdlog::info("UiManager shutdown");
}

void UiManager::initialiseImGui(WindowManager& windowManager) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(windowManager.getWindow(), true);
  ImGui_ImplOpenGL3_Init(getGlVersionString().c_str());

  // Font
  if (!config_.fontPath.empty()) {
    setImGuiFont(config_.fontPath);
  }

  // Style
  if (config_.useDefaultStyle) {
    defaultStyle();
  }

  // DPI
  contentScale_ = window_.getContentScale() * config_.scale;
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(contentScale_); // Widgets/Padding
  style.FontScaleDpi = contentScale_; // Fonts DPI
  style.FontScaleMain = 1.0F;
}

std::string UiManager::getGlVersionString() const {
  const GLVersion glVersion = window_.getOpenGlVersion();
  return "#version " + std::to_string(glVersion.major) + std::to_string(glVersion.minor) + "0";
}

void UiManager::setImGuiFont(const std::string& fontPath) const {
  ImGuiIO& inputOutput = ImGui::GetIO();
  inputOutput.IniFilename = nullptr; // No imgui.ini

  auto foundPath = assets::find(fontPath);
  if (!foundPath) {
    inputOutput.Fonts->AddFontDefault();
    return;
  }

  inputOutput.Fonts->AddFontFromFileTTF(foundPath->c_str(), config_.fontSize);
}

void UiManager::beginFrame() const {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::SetNextWindowSizeConstraints(ImVec2(config_.minWindowWidth * contentScale_, 0.0F),
                                      ImVec2(FLT_MAX, FLT_MAX));
  ImGui::Begin(config_.title.c_str(), nullptr, kMainWindowFlags);
  ImGui::SetWindowPos(ImVec2(kWindowPosX, kWindowPosY), ImGuiCond_Once);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void UiManager::endFrame() const {
  ImGui::End();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UiManager::drawBaseUi(const RootState& state) {
  if (config_.showStatsHeader) {
    drawStatsHeader(state);
  }
  if (config_.showScenesHeader) {
    drawScenesHeader(state);
  }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void UiManager::draw(UiEntry& entry, const RootState& state) {
  const std::string title = entry.title();
  if (title.empty()) {
    entry.onDraw(state);
    return;
  }

  // Wrap in collapsible header if title is set
  const bool open = ImGui::CollapsingHeader(title.c_str());
  if (open) {
    entry.onDraw(state);
  }
}

void UiManager::drawStatsHeader(const RootState& state) {
  if (ImGui::CollapsingHeader("Window")) {
    ImGui::Text("FPS: %.1f", state.fps);
    ImGui::SameLine();
    ImGui::Text("MS: %.2f", state.ms);

    // Event Manager Fullscreen Event
    static bool useFullscreen = false;
    if (ImGui::Checkbox("Fullscreen", &useFullscreen)) {
      // TODO: Implement Fullscreen Event
      spdlog::debug("UiManager: Fullscreen({})", useFullscreen);
    }
  }
}

void UiManager::drawScenesHeader(const RootState& state) {
  if (state.sceneNames.empty()) {
    return;
  }

  if (ImGui::CollapsingHeader("Scenes")) {
    int selected = state.currentSceneIndex;
    for (int i = 0; i < state.sceneNames.size(); ++i) {
      const std::string& sceneName = state.sceneNames[i];
      if (ImGui::RadioButton(sceneName.c_str(), selected == i)) {
        // TODO: Implement SceneChange Event
        spdlog::debug("UiManager: Selected Scene({}, {})", i, sceneName);
        selected = i;
      }
    }
  }
}

// NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
void UiManager::defaultStyle() {
  ImGuiStyle& style = ImGui::GetStyle();

  // Layout
  style.WindowTitleAlign = ImVec2(0.5F, 0.5F);
  style.WindowBorderSize = 0.0F;
  style.WindowPadding = ImVec2(8.0F, 5.0F);
  style.WindowRounding = 4.0F;

  style.FramePadding = ImVec2(5.0F, 5.0F);
  style.FrameRounding = 2.0F;
  style.GrabRounding = 3.0F;

  style.ItemSpacing = ImVec2(5.0F, 5.0F);

  style.ScrollbarSize = 8.0F;
  style.ScrollbarRounding = 12.0F;

  // Colors
  style.Colors[ImGuiCol_TitleBg] = ImColor(60, 60, 60, 255);
  style.Colors[ImGuiCol_TitleBgActive] = ImColor(60, 60, 60, 255);
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImColor(60, 60, 60, 75);

  style.Colors[ImGuiCol_FrameBg] = ImColor(37, 36, 37, 255);
  style.Colors[ImGuiCol_FrameBgActive] = ImColor(37, 36, 37, 255);
  style.Colors[ImGuiCol_FrameBgHovered] = ImColor(37, 36, 37, 255);

  style.Colors[ImGuiCol_Header] = ImColor(50, 50, 50, 125);
  style.Colors[ImGuiCol_HeaderActive] = ImColor(70, 70, 70, 125);
  style.Colors[ImGuiCol_HeaderHovered] = ImColor(70, 70, 70, 255);

  style.Colors[ImGuiCol_Separator] = ImColor(70, 70, 70, 255);
  style.Colors[ImGuiCol_SeparatorActive] = ImColor(76, 76, 76, 255);
  style.Colors[ImGuiCol_SeparatorHovered] = ImColor(76, 76, 76, 255);

  style.Colors[ImGuiCol_Button] = ImColor(31, 30, 31, 255);
  style.Colors[ImGuiCol_ButtonActive] = ImColor(255, 30, 31, 255);
  style.Colors[ImGuiCol_ButtonHovered] = ImColor(41, 40, 41, 255);

  style.Colors[ImGuiCol_CheckMark] = ImColor(230, 28, 43, 255);

  style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.6F, 0.13F, 0.13F, 1.0F);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.7F, 0.23F, 0.23F, 1.0F);
}
// NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)

} // namespace blkhurst
