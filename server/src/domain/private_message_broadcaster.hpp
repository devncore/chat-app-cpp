#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

#include "chat.pb.h"
#include "domain/client_registry.hpp"
#include "service/events/chat_service_events.hpp"

namespace domain {

enum class NextPrivateMessageStatus {
  kOk,
  kNoMessage,
  kPeerMissing,
};

class IPrivateMessageBroadcaster {
public:
  virtual ~IPrivateMessageBroadcaster() = default;

  virtual NextPrivateMessageStatus
  nextPrivateMessage(const std::string &peer, std::chrono::milliseconds waitFor,
                     chat::InformClientsNewMessageResponse &out) = 0;

  virtual bool normalizePrivateMessageIndex(const std::string &peer) = 0;
};

class PrivateMessageBroadcaster : public IPrivateMessageBroadcaster,
                                   public events::IServiceEventObserver {
public:
  explicit PrivateMessageBroadcaster(const ClientRegistry &clientRegistry);

  // IPrivateMessageBroadcaster
  NextPrivateMessageStatus
  nextPrivateMessage(const std::string &peer, std::chrono::milliseconds waitFor,
                     chat::InformClientsNewMessageResponse &out) override;

  bool normalizePrivateMessageIndex(const std::string &peer) override;

  // IServiceEventObserver
  void onClientConnected(const events::ClientConnectedEvent &event) override;
  void
  onClientDisconnected(const events::ClientDisconnectedEvent &event) override;
  void onMessageSent(const events::MessageSentEvent &event) override;
  void
  onPrivateMessageSent(const events::PrivateMessageSentEvent &event) override;

private:
  const ClientRegistry &clientRegistry_;
  mutable std::mutex mutex_;
  std::condition_variable messageCv_;
  // Per-peer message queues: peer -> deque of private messages for that peer
  std::unordered_map<std::string,
                     std::deque<chat::InformClientsNewMessageResponse>>
      peerMessageQueues_;
};

} // namespace domain
