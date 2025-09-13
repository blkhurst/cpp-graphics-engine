## Logger

- [ ] Replace global `spdlog::info(...)` calls.
- [ ] Add `Logger` factory to return named loggers (`[%n] [window]`) with our sinks; prevents clashes.
- [ ] Decide whether to make spdlog internal by:
  - Removing `find_dependency(spdlog ...)` from Config.cmake
  - Linking header only `PRIVATE $<BUILD_INTERFACE:spdlog::spdlog_header_only>`.

## Class Constructors

- [ ] Understand and implement correct copy/move semantics.

## Buffer

- [ ] Make `std::span<T>` const in constructor.
- [ ] Add helper to cast non-const `std::span<T>` to `std::span<const T>`.

## Program

- [ ] Remove `// NOLINT` comments by disabling or resolving:
  - `readability-identifier-length`
  - `bugprone-easily-swappable-parameters`
- [x] Resolve shader/asset paths via executable-relative or CMake-defined asset root; add fallback search paths.
- [ ] Separate out preprocessing logic.
- [ ] **Upgrade to DI + Lazy Build**: Replace `ShaderRegistry` singleton and static `ShaderPreprocessor` with Engine-owned variants, add `Program::ensureBuilt(BuildServices` and call on first use / warmup; deprecate old static-singleton path.

## Layer Mask

- [ ] Add `layerMask` to `Object3D`
- [ ] Add `setCullingMask` and `cullingMask` to `Camera`

## Geometry

- [ ] Allow `setAttribute` to take `BufferAttribute` or `InterleavedBufferAttribute`.
- [ ] Use `std::unordered_map<Attrib, BufferAttribute>` and add utility `calcDrawCount`.

## UI

- [ ] Make global `uuid` utility; add `uuid` to `UiEntry` and use `ImGui::PushID / PopID` to prevent duplicate naming errors.

## API

- [ ] Hide low-level classes from public API:
  - Make `VertexArray` and `Buffer` internal (private headers).
  - Convert `Geometry` to PImpl; forward declare VAO/Buffer in header.
  - Replace `Geometry::vertexArray()` with `Geometry::bind()`.
  - Make dtor virtual and define in source to ensure forwarded types are ok.

## Assets

- [ ] Extend `assetsConfig` with `envVar`, `useCwd`, `useExeDir`, `verbose`.
- [ ] Embed font and load using `AddFontFromMemoryTTF`.

## Engine/Renderer

- [ ] use `vec4` for `clearColor`
- [ ] Clarify that `uResolution` represents `windowFramebufferSize`, not `renderTargetFramebufferSize`, or `viewportSize`
- [ ] Add callbacks & RootState for `windowSize` and `contentScale` (in addition to `windowFramebufferSize`)

## Controllers

- [ ] `OrbitController` does not support `OrthoCamera`.

## Object

- [ ] Make `clone` virtual to avoid duplicating logic in `Mesh`, `PerspectiveCamera`, and `OrthoCamera`.
  - The problem is that you cannot use virtual covariant return types when using smart pointers.
  - `Material` implements `cloneAs` helper, which also cannot be used on unique pointers.
