#pragma once

#include "service/events/chat_service_events.hpp"

#include <vector>

namespace mock {

class MockServiceEventObserver : public events::IServiceEventObserver {
public:
  // Store received events for verification
  std::vector<events::ClientConnectedEvent> clientConnectedEvents;
  std::vector<events::ClientDisconnectedEvent> clientDisconnectedEvents;
  std::vector<events::MessageSentEvent> messageSentEvents;

  void onClientConnected(const events::ClientConnectedEvent &event) override {
    clientConnectedEvents.push_back(event);
  }

  void onClientDisconnected(
      const events::ClientDisconnectedEvent &event) override {
    clientDisconnectedEvents.push_back(event);
  }

  void onMessageSent(const events::MessageSentEvent &event) override {
    messageSentEvents.push_back(event);
  }

  void reset() {
    clientConnectedEvents.clear();
    clientDisconnectedEvents.clear();
    messageSentEvents.clear();
  }

  bool hasReceivedClientConnected() const {
    return !clientConnectedEvents.empty();
  }

  bool hasReceivedClientDisconnected() const {
    return !clientDisconnectedEvents.empty();
  }

  bool hasReceivedMessageSent() const { return !messageSentEvents.empty(); }

  size_t totalEventsReceived() const {
    return clientConnectedEvents.size() + clientDisconnectedEvents.size() +
           messageSentEvents.size();
  }
};

} // namespace mock
