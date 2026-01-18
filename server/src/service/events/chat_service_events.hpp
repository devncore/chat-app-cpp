#pragma once

#include <chrono>
#include <string>

namespace events {

// Event types representing different gRPC events
struct ClientConnectedEvent {
  std::string peer;
  std::string pseudonym;
  std::string gender;
  std::string country;
};

struct ClientDisconnectedEvent {
  std::string peer;
  std::string pseudonym;
  std::chrono::steady_clock::duration connectionDuration;
};

struct MessageSentEvent {
  std::string peer;
  std::string pseudonym;
  std::string content;
};

// Simple virtual interface for event observers
class IServiceEventObserver {
public:
  virtual ~IServiceEventObserver() = default;

  virtual void onClientConnected(const ClientConnectedEvent &event) = 0;
  virtual void onClientDisconnected(const ClientDisconnectedEvent &event) = 0;
  virtual void onMessageSent(const MessageSentEvent &event) = 0;
};

} // namespace events
