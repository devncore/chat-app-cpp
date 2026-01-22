#pragma once

#include "service/events/chat_service_events.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace events {

class EventDispatcher {
public:
  EventDispatcher() = default;
  ~EventDispatcher() = default;

  // Delete copy/move operations
  EventDispatcher(const EventDispatcher &) = delete;
  EventDispatcher &operator=(const EventDispatcher &) = delete;
  EventDispatcher(EventDispatcher &&) = delete;
  EventDispatcher &operator=(EventDispatcher &&) = delete;

  // Register an observer - does not take ownership
  void registerObserver(std::weak_ptr<IServiceEventObserver> observer) {
    std::lock_guard<std::mutex> lock(mutex_);
    observers_.push_back(std::move(observer));
  }

  // Notify all observers of a client connection
  void notifyClientConnected(const ClientConnectedEvent &event) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &weakObserver : observers_) {
      if (auto observer = weakObserver.lock()) {
        observer->onClientConnected(event);
      }
    }
  }

  // Notify all observers of a client disconnection
  void notifyClientDisconnected(const ClientDisconnectedEvent &event) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &weakObserver : observers_) {
      if (auto observer = weakObserver.lock()) {
        observer->onClientDisconnected(event);
      }
    }
  }

  // Notify all observers of a message sent
  void notifyMessageSent(const MessageSentEvent &event) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &weakObserver : observers_) {
      if (auto observer = weakObserver.lock()) {
        observer->onMessageSent(event);
      }
    }
  }

  // Notify all observers of a private message sent
  void notifyPrivateMessageSent(const PrivateMessageSentEvent &event) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &weakObserver : observers_) {
      if (auto observer = weakObserver.lock()) {
        observer->onPrivateMessageSent(event);
      }
    }
  }

private:
  std::mutex mutex_;
  std::vector<std::weak_ptr<IServiceEventObserver>> observers_;
};

} // namespace events
