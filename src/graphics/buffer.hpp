#pragma once

#include <cstdint>
#include <span>

namespace blkhurst {

class Buffer {
public:
  Buffer(const void* data, intptr_t sizeBytes, bool dynamic = false);
  ~Buffer();

  Buffer(const Buffer&) = delete;
  Buffer(Buffer&&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  Buffer& operator=(Buffer&&) = delete;

  void setData(const void* data, intptr_t sizeBytes, bool dynamic = false);
  void setSubData(intptr_t offsetBytes, const void* data, intptr_t sizeBytes);

  // Convenience using std::span
  template <class T>
  Buffer(std::span<T> data, bool dynamic = false)
      : Buffer(data.data(), data.size_bytes(), dynamic) {
  }
  template <class T> void setData(std::span<T> data, bool dynamic = false) {
    setData(data.data(), data.size_bytes(), dynamic);
  }
  template <class T> void setSubData(intptr_t elemOffset, std::span<T> data) {
    setSubData(elemOffset * sizeof(T), static_cast<const void*>(data.data()), data.size_bytes());
  }

  [[nodiscard]] unsigned int id() const {
    return id_;
  }
  [[nodiscard]] intptr_t size() const {
    return size_;
  }

private:
  unsigned int id_ = 0;
  intptr_t size_ = 0;
};

} // namespace blkhurst
