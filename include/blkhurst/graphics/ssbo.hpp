#pragma once
#include <span>
#include <vector>

namespace blkhurst {

class SSBO {
public:
  SSBO(unsigned bindingIndex);
  ~SSBO();

  SSBO(const SSBO&) = delete;
  SSBO& operator=(const SSBO&) = delete;
  SSBO(SSBO&&) = delete;
  SSBO& operator=(SSBO&&) = delete;

  [[nodiscard]] unsigned id() const;
  [[nodiscard]] unsigned binding() const;
  [[nodiscard]] std::size_t capacityBytes() const;

  template <class T> void update(const T& pod) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "SSBO::update requires trivially-copyable (std430) type");
    setData(&pod, sizeof(T));
  }
  template <class T> void updateArray(const std::span<T> data) {
    static_assert(std::is_trivially_copyable_v<T>,
                  "SSBO::updateArray requires trivially-copyable (std430) type");
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
  static void barrier();

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
