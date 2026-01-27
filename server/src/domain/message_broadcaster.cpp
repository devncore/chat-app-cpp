#include "domain/message_broadcaster.hpp"

namespace domain {

MessageBroadcaster::MessageBroadcaster(const ClientRegistry &clientRegistry)
    : clientRegistry_(clientRegistry) {}

NextMessageStatus
MessageBroadcaster::nextMessage(std::string_view peer,
                                std::chrono::milliseconds waitFor,
                                chat::InformClientsNewMessageResponse &out) {
  std::unique_lock<std::mutex> lock(mutex_);
  const std::string peerKey(peer);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerIndices_.erase(peerKey);
    return NextMessageStatus::kPeerMissing;
  }

  auto it = peerIndices_.find(peerKey);
  if (it == peerIndices_.end()) {
    it = peerIndices_.emplace(peerKey, messageHistory_.size()).first;
  }

  if (it->second < messageHistory_.size()) {
    out = messageHistory_[it->second];
    ++it->second;
    return NextMessageStatus::kOk;
  }

  messageCv_.wait_for(lock, waitFor);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerIndices_.erase(peerKey);
    return NextMessageStatus::kPeerMissing;
  }

  it = peerIndices_.find(peerKey);
  if (it == peerIndices_.end()) {
    return NextMessageStatus::kPeerMissing;
  }

  if (it->second < messageHistory_.size()) {
    out = messageHistory_[it->second];
    ++it->second;
    return NextMessageStatus::kOk;
  }

  return NextMessageStatus::kNoMessage;
}

bool MessageBroadcaster::normalizeMessageIndex(std::string_view peer) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string peerKey(peer);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerIndices_.erase(peerKey);
    return false;
  }

  auto it = peerIndices_.find(peerKey);
  if (it == peerIndices_.end()) {
    peerIndices_[peerKey] = messageHistory_.size();
    return true;
  }

  if (it->second > messageHistory_.size()) {
    it->second = messageHistory_.size();
  }

  return true;
}

void MessageBroadcaster::onClientConnected(
    [[maybe_unused]] const events::ClientConnectedEvent &event) {}

void MessageBroadcaster::onClientDisconnected(
    [[maybe_unused]] const events::ClientDisconnectedEvent &event) {}

void MessageBroadcaster::onMessageSent(const events::MessageSentEvent &event) {
  chat::InformClientsNewMessageResponse payload;
  payload.set_author(event.pseudonym);
  payload.set_content(event.content);

  {
    std::lock_guard<std::mutex> lock(mutex_);
    messageHistory_.push_back(payload);
  }

  messageCv_.notify_all();
}

void MessageBroadcaster::onPrivateMessageSent(
    [[maybe_unused]] const events::PrivateMessageSentEvent &event) {}

} // namespace domain
