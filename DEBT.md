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
- [ ] Resolve shader/asset paths via executable-relative or CMake-defined asset root; add fallback search paths.
