#include <blkhurst/events/event_bus.hpp>
#include <spdlog/spdlog.h>

namespace blkhurst {

void Subscription::unsubscribe() {
  if (!valid()) {
    return;
  }
  if (bus_->unsubscribe(type_, id_)) {
    bus_ = nullptr;
    id_ = EventBusConst::kInvalidListenerId;
  }
}

bool EventBus::unsubscribe(std::type_index type, std::uint64_t listenerId) {
  std::scoped_lock lock(mutex_);
  auto foundType = listenersByType_.find(type);
  if (foundType == listenersByType_.end()) {
    return false;
  }

  // Iterate listeners and remove by id
  auto& listeners = foundType->second;
  const auto removed = std::erase_if(
      listeners, [listenerId](const Listener& listener) { return listener.id == listenerId; });

  if (removed == 0) {
    return false;
  }

  // Drop the type key if no listeners remain
  if (listeners.empty()) {
    listenersByType_.erase(foundType);
  }

  spdlog::trace("EventBus unsubscribed id={} type={}", listenerId, type.name());
  return true;
}

std::size_t EventBus::listenerCount(std::type_index type) const {
  std::scoped_lock lock(mutex_);
  auto foundType = listenersByType_.find(type);
  return (foundType == listenersByType_.end()) ? 0U : foundType->second.size();
}

std::size_t EventBus::totalListenerCount() const {
  std::scoped_lock lock(mutex_);
  std::size_t total = 0;
  for (const auto& typeAndListener : listenersByType_) {
    total += typeAndListener.second.size();
  }
  return total;
}

} // namespace blkhurst
