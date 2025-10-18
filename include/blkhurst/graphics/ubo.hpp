#pragma once
#include <cstddef>
#include <span>
#include <vector>

namespace blkhurst {

class UBO {
public:
  UBO(unsigned bindingIndex);
  ~UBO();

  UBO(const UBO&) = delete;
  UBO& operator=(const UBO&) = delete;
  UBO(UBO&&) = delete;
  UBO& operator=(UBO&&) = delete;

  [[nodiscard]] unsigned id() const;
  [[nodiscard]] unsigned binding() const;
  [[nodiscard]] std::size_t capacityBytes() const;

  template <class T> void update(const T& pod) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "UBO::update requires trivially-copyable (std140 padded) type");
    setData(&pod, sizeof(T));
  }
  template <class T> void updateArray(const std::span<T> data) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "UBO::updateArray requires trivially-copyable (std140 padded) type");
    setData(data.data(), data.size_bytes());
  }

  template <class T> T read() const {
    T out{};
    getData(&out, sizeof(T));
    return out;
  }
  template <class T> std::vector<T> readArray(std::size_t count) const {
    std::vector<T> out(count);
    getData(out.data(), out.size() * sizeof(T));
    return out;
  }

  void bind() const;

private:
  unsigned id_ = 0;
  unsigned binding_;
  std::size_t capacity_ = 0;

  void setData(const void* data, size_t sizeBytes);
  void setSubData(size_t offsetBytes, const void* data, size_t sizeBytes) const;

  void getData(void* data, size_t sizeBytes) const;
  void getSubData(size_t offsetBytes, void* data, size_t sizeBytes) const;
};

} // namespace blkhurst
