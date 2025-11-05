#pragma once
#include <cstdint>
#include <string>

namespace blkhurst {

using UUID = std::uint64_t;
static constexpr UUID kInvalidUUID = 0;

class Identifiable {
public:
  Identifiable();

  [[nodiscard]] UUID uuid() const;
  [[nodiscard]] const std::string& uuidString() const; // name if set, else hex representation
  [[nodiscard]] const std::string& name() const;

  void setName(std::string name);

private:
  UUID uuid_;
  std::string hex_;
  std::string name_;

  static UUID uuidMake();
  static std::string uuidHex(UUID uuid);
};

} // namespace blkhurst
