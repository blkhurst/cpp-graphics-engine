#include <blkhurst/util/identifiable.hpp>

#include <iomanip>
#include <random>
#include <spdlog/spdlog.h>
#include <sstream>

namespace blkhurst {

Identifiable::Identifiable()
    : uuid_(uuidMake()),
      hex_(uuidHex(uuid_)) {
}

UUID Identifiable::uuid() const {
  return uuid_;
}

const std::string& Identifiable::uuidString() const {
  if (!name_.empty()) {
    return name_;
  }
  return hex_;
}

const std::string& Identifiable::name() const {
  return name_;
}

void Identifiable::setName(std::string name) {
  spdlog::trace("Identifiable({}) setName '{}'", uuidString(), name);
  name_ = std::move(name);
}

UUID Identifiable::uuidMake() {
  static std::mt19937_64 rng{std::random_device{}()};
  static std::uniform_int_distribution<UUID> dist;
  return dist(rng);
}

std::string Identifiable::uuidHex(UUID uuid) {
  std::ostringstream stream;
  constexpr int kUuidHexWidth = 16; // 64 bits = 16 hex digits
  stream << std::hex << std::setfill('0') << std::nouppercase << std::setw(kUuidHexWidth) << uuid;
  return stream.str();
}

} // namespace blkhurst
