#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "chat.pb.h"
#include "domain/client_registry.hpp"
#include "service/events/chat_service_events.hpp"

namespace domain {

enum class NextMessageStatus {
  kOk,
  kNoMessage,
  kPeerMissing,
};

class IMessageBroadcaster {
public:
  virtual ~IMessageBroadcaster() = default;

  virtual NextMessageStatus
  nextMessage(std::string_view peer, std::chrono::milliseconds waitFor,
              chat::InformClientsNewMessageResponse &out) = 0;

  virtual bool normalizeMessageIndex(std::string_view peer) = 0;
};

class MessageBroadcaster : public IMessageBroadcaster,
                           public events::IServiceEventObserver {
public:
  explicit MessageBroadcaster(const ClientRegistry &clientRegistry);

  // IMessageBroadcaster
  NextMessageStatus nextMessage(std::string_view peer,
                                std::chrono::milliseconds waitFor,
                                chat::InformClientsNewMessageResponse &out) override;

  bool normalizeMessageIndex(std::string_view peer) override;

  // IServiceEventObserver
  void onClientConnected(const events::ClientConnectedEvent &event) override;
  void onClientDisconnected(const events::ClientDisconnectedEvent &event) override;
  void onMessageSent(const events::MessageSentEvent &event) override;
  void onPrivateMessageSent(const events::PrivateMessageSentEvent &event) override;

private:
  const ClientRegistry &clientRegistry_;
  mutable std::mutex mutex_;
  std::condition_variable messageCv_;
  std::vector<chat::InformClientsNewMessageResponse> messageHistory_;
  std::unordered_map<std::string, std::size_t> peerIndices_;
};

} // namespace domain
