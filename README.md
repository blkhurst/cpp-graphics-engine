# Blkhurst Graphics Engine

A modern C++20 3D graphics engine built with OpenGL, designed for real-time rendering with physically-based rendering (PBR) and image-based lighting (IBL) support.

## Features

- **Modern C++20**: Leverages latest C++ features for clean, efficient code
- **Physically-Based Rendering (PBR)**: Industry-standard material system with metallic-roughness workflow
- **Image-Based Lighting (IBL)**: Environment maps with pre-filtered radiance and irradiance
- **Scene Graph**: Hierarchical object system with transformations
- **Camera System**: Perspective and orthographic cameras with orbital and fly controllers
- **Rich Geometry Primitives**: Built-in shapes including box, sphere, plane, cylinder, torus, capsule, and more
- **Material System**: Extensible material framework with custom shaders and uniforms
- **Lighting**: Support for directional, point, and ambient lights
- **Asset Loading**: Model loading via Assimp, texture loading with STB Image
- **ImGui Integration**: Built-in UI framework for debugging and tools
- **Flexible Configuration**: Comprehensive engine configuration system
- **Tone Mapping**: Multiple tone mapping operators (Linear, Neutral, ACES)
- **Color Spaces**: Support for linear and sRGB output

## Dependencies

The engine requires the following dependencies:

- **OpenGL**: 3.3 or higher
- **GLM** (1.0.1+): Mathematics library
- **GLFW** (3.4+): Window and input management
- **spdlog** (1.15.3+): Logging library
- **Assimp** (6.0.2+): 3D model loading
- **GLAD**: OpenGL loader (bundled)
- **ImGui**: Immediate mode GUI (bundled)
- **STB Image**: Image loading (bundled)

Dependencies can be automatically fetched via CMake FetchContent or provided externally.

## Building

### Requirements

- CMake 3.26 or higher
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- OpenGL development libraries

### Build with FetchContent (Recommended)

```bash
mkdir build
cd build
cmake .. -DBLKHURST_USE_FETCHCONTENT=ON
cmake --build .
```

### Build with System Dependencies

If you have the dependencies installed on your system:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### CMake Options

- `BLKHURST_USE_FETCHCONTENT`: Automatically fetch dependencies (default: OFF)
- `BLKHURST_BUILD_EXAMPLES`: Build example applications (default: ON)
- `BLKHURST_INSTALL`: Generate installation target (default: ON)

## Quick Start

Here's a minimal example to get started:

```cpp
#include <blkhurst/engine.hpp>
#include <blkhurst/engine/config.hpp>

int main() {
  // Configure the engine
  blkhurst::EngineConfig config;
  config.loggerConfig.level = blkhurst::LogLevel::debug;
  config.windowConfig.title = "My Graphics App";
  
  // Create and run the engine
  blkhurst::Engine engine{config};
  engine.run();
  
  return 0;
}
```

### Creating a Custom Scene

```cpp
#include <blkhurst/engine.hpp>
#include <blkhurst/scene/scene.hpp>
#include <blkhurst/objects/mesh.hpp>
#include <blkhurst/geometry/sphere_geometry.hpp>
#include <blkhurst/materials/pbr_material.hpp>
#include <blkhurst/cameras/perspective_camera.hpp>
#include <blkhurst/lights/directional_light.hpp>

class MyScene : public blkhurst::Scene {
public:
  void onStart(const blkhurst::RootState& state) override {
    // Create a camera
    auto camera = std::make_shared<blkhurst::PerspectiveCamera>(
      45.0f, state.aspectRatio, 0.1f, 100.0f
    );
    camera->setPosition({0, 0, 5});
    setActiveCamera(camera);
    
    // Create a sphere with PBR material
    auto geometry = std::make_shared<blkhurst::SphereGeometry>(1.0f);
    auto material = std::make_shared<blkhurst::PBRMaterial>();
    material->setColor({0.8f, 0.2f, 0.2f});
    material->setMetalness(0.5f);
    material->setRoughness(0.3f);
    
    auto sphere = std::make_unique<blkhurst::Mesh>(geometry, material);
    add(std::move(sphere));
    
    // Add a directional light
    auto light = std::make_unique<blkhurst::DirectionalLight>();
    light->setDirection({-1, -1, -1});
    light->setIntensity(2.0f);
    add(std::move(light));
  }
};

int main() {
  blkhurst::EngineConfig config;
  config.windowConfig.title = "Custom Scene";
  
  blkhurst::Engine engine{config};
  engine.registerScene<MyScene>("main");
  engine.setScene("main");
  engine.run();
  
  return 0;
}
```

## Architecture

### Core Components

- **Engine**: Main engine class that manages the rendering loop and subsystems
- **Scene**: Container for 3D objects, cameras, and lights with lifecycle hooks
- **Renderer**: OpenGL renderer with support for render targets and post-processing
- **Object3D**: Base class for all scene objects with transformation hierarchy
- **Mesh**: Combination of geometry and material
- **Camera**: View frustum and projection (Perspective/Orthographic)
- **Material**: Shader programs and rendering state
- **Geometry**: Vertex data and attributes
- **Lights**: Scene illumination (Directional/Point/Ambient)

### Project Structure

```
cpp-graphics-engine/
├── include/blkhurst/       # Public API headers
│   ├── engine/            # Engine core and configuration
│   ├── scene/             # Scene management
│   ├── renderer/          # Rendering system
│   ├── objects/           # 3D objects (Mesh, Object3D)
│   ├── cameras/           # Camera implementations
│   ├── controllers/       # Camera controllers
│   ├── geometry/          # Geometry primitives
│   ├── materials/         # Material system
│   ├── shaders/           # Shader management
│   ├── lights/            # Light types
│   ├── textures/          # Texture handling
│   ├── loaders/           # Asset loaders
│   ├── ibl/               # Image-based lighting
│   ├── ui/                # ImGui integration
│   ├── events/            # Event system
│   ├── input/             # Input handling
│   └── util/              # Utilities
├── src/                   # Implementation files
├── examples/              # Example applications
├── dependencies/          # Bundled dependencies (GLAD, ImGui, STB)
└── CMakeLists.txt         # Build configuration
```

## Materials

The engine supports multiple material types:

- **PBRMaterial**: Physically-based rendering with metallic-roughness workflow
- **BasicMaterial**: Simple unlit material with color and texture support
- **SkyboxMaterial**: Environment mapping for skyboxes

## Geometry Primitives

Built-in geometry generators:

- BoxGeometry
- SphereGeometry
- PlaneGeometry
- CylinderGeometry
- TorusGeometry
- TorusKnotGeometry
- CapsuleGeometry
- CircleGeometry
- BilinearQuadGeometry

## Lighting

Supported light types:

- **DirectionalLight**: Parallel light rays (like sunlight)
- **PointLight**: Omnidirectional point source with attenuation
- **AmbientLight**: Uniform ambient illumination

## Camera Controllers

Interactive camera control:

- **OrbitController**: Orbit around a target point with mouse/touch
- **FlyController**: Free-flying first-person camera

## Asset Loading

The engine supports loading:

- **3D Models**: Via Assimp (GLTF, OBJ, FBX, and many more formats)
- **Textures**: PNG, JPG, TGA, BMP, HDR via STB Image
- **Cube Maps**: For environment mapping and skyboxes

## Configuration

The engine can be configured via `EngineConfig`:

```cpp
blkhurst::EngineConfig config;

// Logging
config.loggerConfig.level = blkhurst::LogLevel::info;

// Window
config.windowConfig.title = "My Application";
config.windowConfig.width = 1280;
config.windowConfig.height = 720;
config.windowConfig.vsync = true;
config.windowConfig.resizable = true;

// Assets
config.assetsConfig.rootPath = "./assets";

// UI
config.uiConfig.enabled = true;
```

## Installation

To install the library:

```bash
cmake --build build --target install
```

Then use in your CMake project:

```cmake
find_package(BlkhurstEngine REQUIRED)
target_link_libraries(your_target PRIVATE BlkhurstEngine)
```

## Technical Details

- **Rendering API**: OpenGL 3.3+
- **Shader Language**: GLSL (with preprocessing support)
- **Coordinate System**: Right-handed Y-up
- **Color Space**: Linear rendering with optional sRGB output
- **Texture Format**: Automatic format detection and conversion

## Development Status

This is an actively developed project. See `DEBT.md` for known technical debt and planned improvements.

## Examples

The `examples/` directory contains sample applications demonstrating various engine features. Build with `BLKHURST_BUILD_EXAMPLES=ON` (enabled by default).

## Contributing

This is a personal project, but contributions are welcome! Please ensure code follows the existing style (see `.clang-format` and `.clang-tidy`).

## License

[License information to be added]

## Acknowledgments

- **ImGui**: Omar Cornut and contributors
- **GLM**: G-Truc Creation
- **GLFW**: Marcus Geelnard, Camilla Löwy, and contributors
- **Assimp**: Open Asset Import Library team
- **spdlog**: Gabi Melman
- **STB**: Sean Barrett
