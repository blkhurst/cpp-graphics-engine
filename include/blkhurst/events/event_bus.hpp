#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

/**
EventBus
  - subscribe<T>(fn)  - Register a callback
  - post<T>(event)    - Trigger an already constructed event
  - emit<T>(args...)  - Construct event in place and trigger

  - Thread-safe
  - Synchronous delivery, in registration order
  - Subscription destructor unsubscribes (RAII)
*/

namespace blkhurst {

class EventBus;
struct EventBusConst {
  static constexpr std::uint64_t kInvalidListenerId = 0ULL;
};

// Subscription - Unsubscribes in destructor (RAII)
class Subscription {
public:
  // Constructors
  Subscription() = default;
  Subscription(EventBus* bus, std::type_index type, std::uint64_t uuid)
      : bus_(bus),
        type_(type),
        id_(uuid) {
  }

  // Destructor
  ~Subscription() {
    unsubscribe();
  }

  // Non-copyable
  Subscription(const Subscription&) = delete;
  Subscription& operator=(const Subscription&) = delete;

  // Moveable Constructor
  Subscription(Subscription&& other) noexcept
      : bus_(other.bus_),
        type_(other.type_),
        id_(other.id_) {
    // Invalidate the moved-from object so it won't unsubscribe again.
    other.bus_ = nullptr;
    other.id_ = EventBusConst::kInvalidListenerId;
  }

  // Moveable Assignment
  Subscription& operator=(Subscription&& other) noexcept {
    // If moving new subscription into existing, clean-up.
    if (this != &other) {
      // Drop any current subscription we own.
      unsubscribe();

      // Steal other's state.
      bus_ = other.bus_;
      type_ = other.type_;
      id_ = other.id_;

      // Invalidate the moved-from object so it won't unsubscribe again.
      other.bus_ = nullptr;
      other.id_ = EventBusConst::kInvalidListenerId;
    }
    return *this;
  }

  void unsubscribe();
  [[nodiscard]] bool valid() const {
    return bus_ != nullptr && id_ != EventBusConst::kInvalidListenerId;
  }

private:
  EventBus* bus_ = nullptr;
  std::type_index type_{typeid(void)};
  std::uint64_t id_ = EventBusConst::kInvalidListenerId;
};

// EventBus - Event subscription and dispatcher
class EventBus {
public:
  EventBus() = default;
  ~EventBus() = default;

  EventBus(const EventBus&) = delete;
  EventBus& operator=(const EventBus&) = delete;
  EventBus(EventBus&&) = delete;
  EventBus& operator=(EventBus&&) = delete;

  template <class T, class Fn> Subscription subscribe(Fn&& callback) {
    // Ensure callback type can be called as `const T&`
    using Decayed = std::decay_t<Fn>;
    static_assert(std::is_invocable_v<Decayed, const T&>,
                  "Callback must be callable as void(const T&).");

    const std::type_index key{typeid(T)};
    const std::uint64_t listenerId = nextId_.fetch_add(1, std::memory_order_relaxed);

    // Erase type to store as `const void*`
    ErasedFunction erased = [func = std::function<void(const T&)>(std::forward<Fn>(callback))](
                                const void* eventPointer) {
      func(*static_cast<const T*>(eventPointer));
    };

    // Thread-safe store listener for this event type
    {
      std::scoped_lock lock(mutex_);
      auto& list = listenersByType_[key];
      list.push_back(Listener{listenerId, std::move(erased)});
    }

    // Return Subscription, caller responsible for keep-alive
    return Subscription{this, key, listenerId};
  }

  template <class T> void post(const T& event) const {
    const std::type_index key{typeid(T)};

    // Copy listeners to allow unsubscribe during iteration
    std::vector<Listener> snapshot;
    {
      std::scoped_lock lock(mutex_);
      auto foundType = listenersByType_.find(key);
      if (foundType == listenersByType_.end()) {
        // No listeners for this event type
        return;
      }
      snapshot = foundType->second;
    }

    // Deliver event to each listener
    for (const auto& listener : snapshot) {
      listener.function(&event);
    }
  }

  // Construct an event of type T in place and deliver it
  template <class T, class... Args> void emit(Args&&... args) const {
    T event{std::forward<Args>(args)...};
    post(event);
  }

  [[nodiscard]] std::size_t listenerCount(std::type_index type) const;
  [[nodiscard]] std::size_t totalListenerCount() const;

private:
  // Allow Subscription to call unsubscribe
  friend class Subscription;
  bool unsubscribe(std::type_index type, std::uint64_t listenerId);

  // Protects listenersByType_ (thread-safe add/remove/iterate; atomic counter for ids)
  mutable std::mutex mutex_;
  std::atomic<std::uint64_t> nextId_{EventBusConst::kInvalidListenerId + 1};

  // Store id + erased type function for each listener
  using ErasedFunction = std::function<void(const void*)>;
  struct Listener {
    std::uint64_t id;
    ErasedFunction function;
  };
  std::unordered_map<std::type_index, std::vector<Listener>> listenersByType_;
};

} // namespace blkhurst
