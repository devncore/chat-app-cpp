#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "chat.pb.h"
#include "domain/client_registry.hpp"

namespace domain {

enum class NextClientEventStatus {
  kOk,
  kNoEvent,
  kPeerMissing,
};

class IClientEventBroadcaster {
public:
  virtual ~IClientEventBroadcaster() = default;

  virtual void
  broadcastClientEvent(const std::string &pseudonym,
                       chat::ClientEventData_ClientEventType eventType) = 0;

  virtual NextClientEventStatus
  nextClientEvent(const std::string &peer, std::chrono::milliseconds waitFor,
                  chat::ClientEventData *out) = 0;

  virtual bool normalizeClientEventIndex(const std::string &peer) = 0;
};

class ClientEventBroadcaster : public IClientEventBroadcaster,
                               public events::IServiceEventObserver {
public:
  explicit ClientEventBroadcaster(const ClientRegistry &clientRegistry);

  void broadcastClientEvent(
      const std::string &pseudonym,
      chat::ClientEventData_ClientEventType eventType) override;

  NextClientEventStatus
  nextClientEvent(const std::string &peer, std::chrono::milliseconds waitFor,
                  chat::ClientEventData *out) override;

  bool normalizeClientEventIndex(const std::string &peer) override;

  // IServiceEventObserver interface
  void onClientConnected(const events::ClientConnectedEvent &event) override;
  void onClientDisconnected(const events::ClientDisconnectedEvent &event) override;
  void onMessageSent(const events::MessageSentEvent &event) override;

private:
  const ClientRegistry &clientRegistry_;
  mutable std::mutex mutex_;
  std::condition_variable clientEventCv_;
  std::vector<chat::ClientEventData> clientEvents_;
  std::unordered_map<std::string, std::size_t> peerIndices_;
};

} // namespace domain
