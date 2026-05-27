# Blkhurst Graphics Engine

A modern C++20 3D graphics engine built with OpenGL DSA and an intuitive scene-graph API.

## Features

#### Highlights

- **Physically-Based Rendering (PBR)** - Lighting model using the metallic-roughness workflow (Cook-Torrance GGX).
- **Image-Based Lighting (IBL)** - PMREM based.
  - **Diffuse Irradiance** - Cosine-weighted convolution or Spherical Harmonics.
  - **Specular** - Prefiltered radiance + BRDF LUT (GGX split-sum approximation).
- **Model & Texture Loading** - Supports .GLB/.FBX/.OBJ, embedded or external textures, sRGB/HDR.
- **Async Asset Pipeline** - Background decode/import; main-thread GPU upload via `ThreadDispatcher`.
- **Loading Screen & Policy** - Configurable overlay; `Preload`, `OnDemand` and `OnDemandUnloadInactive`.
- **Shader Preprocessor** - Composable chunks via `#include`/`#define` directives; sourced via registry or filesystem.
- **Normal Mapping** - Per-fragment tangent-space TBN from screen-space derivatives; flat shading via `dFdx/dFdy`.
- **Tone Mapping (HDR)** - Linear, Neutral, ACES.
- **Color Space & Gamma** - Linear or **sRGB** decoding/sampling and output.
- **Post-processing (WIP)** - Multi-pass chained effects via dual render targets (ping-pong).

#### Core

- **Scene Graph** - Hierarchical nodes with local/world transforms.
- **Cameras & Controllers** - Orthographic & Perspective; Orbit & Fly controllers.
- **Lifecycle Hooks** - `onStart`, `onAttach`, `onUpdate`, `onDetach`.
- **UI** - Built-in scene controls; extensible per-scene/global panels via `UiEntry::onDraw`; DPI aware.
- **Input** - Query keyboard/mouse state. `keyPressed`, `keyDown`, `keyReleased`...
- **Event Bus** - Type-safe publish/subscribe events with scoped (RAII) subscriptions.
- **Asset Resolver** - Configurable search paths (current/executable/install roots).
- **Textures & Cubemaps** - sRGB/linear, mip generation, wrap/filter modes.
- **Framebuffers & Render Targets** - Custom targets via `setRenderTarget`.
- **Instancing (WIP)** - Render many instances of a mesh with a single draw call.
- **Tessellation (WIP)** - Adaptive subdivision via TCS/TES.
- **Compute Shaders (WIP)** - General purpose GPU compute passes.
- **Uniforms & Buffers** - Prefers UBOs/SSBOs; per-draw uniform updates are minimised via caching.
- **Lights** - Ambient, Point, and Directional.
- **Helpers** - Equirectangular to Cubemap, `setBackground` (Skybox), `setEnvironment` (IBL).
- **Built-in Primitives** - Plane, Circle, Box, Sphere, Cylinder, Capsule, Torus, Torus Knot, Bilinear Quad.
- **Cross-platform** - Windows & Linux.

## Dependencies

- **OpenGL 4.5** - Core (_macOS not supported_)
- [**GLFW 3.4**](https://github.com/glfw/glfw) - Windowing & Input
- [**GLM 1.0.1**](https://github.com/g-truc/glm) - Mathematics Library
- [**Assimp 6.0.2**](https://github.com/assimp/assimp) - 3D Model Loading
- [**spdlog 1.15.3**](https://github.com/gabime/spdlog) - Logging Library
- [**GLAD 2**](https://github.com/Dav1dde/glad) - OpenGL Loader (bundled)
- [**ImGui v1.92.2b**](https://github.com/ocornut/imgui) - UI (bundled)
- [**stb**](https://github.com/nothings/stb) - Image Loading (bundled)

Dependencies can be automatically fetched via CMake FetchContent or provided externally.

## Building

### Requirements

- **CMake 3.26**
- **OpenGL Development Libraries**

### Build with FetchContent (Recommended)

```bash
cmake -S . -B build -DBLKHURST_USE_FETCHCONTENT=ON
cmake --build build

# Run the example:
./build/examples/example
```

#### CMake Options

- `BLKHURST_USE_FETCHCONTENT`: Automatically fetch dependencies (default: OFF)
- `BLKHURST_BUILD_EXAMPLES`: Build example applications (default: ON)
- `BLKHURST_INSTALL`: Generate installation target (default: ON)

### NVIDIA OptiX (optional)

Install the requirements:

- [OptiX SDK](https://developer.nvidia.com/designworks/optix/download)
- [CUDA Toolkit](https://developer.nvidia.com/cuda-downloads)
  - or `sudo apt install nvidia-cuda-toolkit`

```bash
cmake -B build-optix \
  -DBLKHURST_ENABLE_OPTIX=ON \
  -DOPTIX_SDK_DIR=/path/to/NVIDIA-OptiX-SDK \

cmake --build build-optix/
```

## Installation

```bash
sudo cmake --install build
```

Use in your CMake project:

```cmake
find_package(BlkhurstEngine 0.1.0 REQUIRED)
target_link_libraries(myapp PRIVATE BlkhurstEngine)
```

## Architecture

### Core Idea

- **Engine**\
  Manages the rendering loop and subsystems (clock, events, window, scene, ui, input, renderer, assets, loading).
- **Object3D**\
  Base class for all 3D objects supporting local/global TRS transforms and children. (scene, camera, mesh, lights)
- **Scene**\
  Root node for a self-contained graph with an active Camera and optional Controller. Provides hooks - `onStart`, `onAttach`, `onUpdate`, `onDetach` - to add custom logic where needed.
- **Mesh** (Geometry + Material)
  - **Geometry** - VertexArray, Buffers, and draw information.
  - **Material** - Compiled shader Program, uniforms, and PipelineState.
- **Renderer**\
  Traverses the Scene, builds per-frame UBOs, collects renderables and sorts by camera distance, binds state, and issues GL draws. Outputs to screen or a RenderTarget.
- **EffectComposer**\
  Using the `Renderer` and a collection of passes to ping-pong between two render targets (A/B), allowing for chained post-processing effects.

### Project Structure

```bash
cpp-graphics-engine/
├── include/blkhurst      # Public API headers
│   ├── assets            # Async AssetLoader
│   ├── cameras           # Camera implementations
│   ├── controllers       # Camera controllers (Orbit/Fly)
│   ├── engine            # Engine PImpl/configuration/RootState
│   ├── events            # Event Bus/Types
│   ├── geometry          # Geometry class and primitives
│   ├── graphics          # Low-level GPU API (VAO, Buffer, Program, UBO/SSBO)
│   ├── ibl               # PMREMGenerator
│   ├── input             # Input system
│   ├── lights            # Ambient/Point/Directional lights
│   ├── loaders           # Texture/CubeTexture/Model loaders
│   ├── materials         # Basic/Normal/Pbr materials, standardised uniform naming, UvTransform
│   ├── model             # ModelProcessor (Assimp to SceneGraph)
│   ├── objects           # Object3D, Mesh
│   ├── postprocessing    # EffectComposer, Built-in Passes
│   ├── renderer          # Renderer, UniformBlocks, RenderTargets, EnvironmentBundle
│   ├── scene             # Scene, SceneManager
│   ├── shaders           # ShaderPreprocessor, ShaderRegistry, Built-in shader sources/chunks,
│   ├── textures          # Texture, CubeTexture
│   ├── ui                # UiEntry, UiManager
│   └── util              # AssetResolver, Identifiable
├── dependencies          # Vendored dependencies (GLAD, ImGui, stb)
├── examples              # Example application
└── src                   # Implementation for the public headers
```

## Known Issues

- **Controllers & OrthographicCamera** - Controllers currently target perspective cameras only.
- **Windows + Assimp + ccache** - If build fails due to an unquoted path containing spaces, disable ccache for Assimp:
  ```cmake
  set(ASSIMP_BUILD_USE_CCACHE OFF CACHE BOOL "" FORCE)
  ```
