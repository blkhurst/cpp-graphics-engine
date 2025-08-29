## Logger

- [ ] Replace global `spdlog::info(...)` calls.
- [ ] Add `Logger` factory to return named loggers (`[%n] [window]`) with our sinks; prevents clashes.
- [ ] Decide whether to make spdlog internal by:
  - Removing `find_dependency(spdlog ...)` from Config.cmake
  - Linking header only `PRIVATE $<BUILD_INTERFACE:spdlog::spdlog_header_only>`.
