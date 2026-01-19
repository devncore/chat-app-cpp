#include "domain/message_broadcaster.hpp"

namespace domain {

MessageBroadcaster::MessageBroadcaster(const ClientRegistry &clientRegistry)
    : clientRegistry_(clientRegistry) {}

NextMessageStatus
MessageBroadcaster::nextMessage(const std::string &peer,
                                std::chrono::milliseconds waitFor,
                                chat::InformClientsNewMessageResponse *out) {
  std::unique_lock<std::mutex> lock(mutex_);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerIndices_.erase(peer);
    return NextMessageStatus::kPeerMissing;
  }

  auto it = peerIndices_.find(peer);
  if (it == peerIndices_.end()) {
    it = peerIndices_.emplace(peer, messageHistory_.size()).first;
  }

  if (it->second < messageHistory_.size()) {
    if (out != nullptr) {
      *out = messageHistory_[it->second];
    }
    ++it->second;
    return NextMessageStatus::kOk;
  }

  messageCv_.wait_for(lock, waitFor);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerIndices_.erase(peer);
    return NextMessageStatus::kPeerMissing;
  }

  it = peerIndices_.find(peer);
  if (it == peerIndices_.end()) {
    return NextMessageStatus::kPeerMissing;
  }

  if (it->second < messageHistory_.size()) {
    if (out != nullptr) {
      *out = messageHistory_[it->second];
    }
    ++it->second;
    return NextMessageStatus::kOk;
  }

  return NextMessageStatus::kNoMessage;
}

bool MessageBroadcaster::normalizeMessageIndex(const std::string &peer) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerIndices_.erase(peer);
    return false;
  }

  auto it = peerIndices_.find(peer);
  if (it == peerIndices_.end()) {
    peerIndices_[peer] = messageHistory_.size();
    return true;
  }

  if (it->second > messageHistory_.size()) {
    it->second = messageHistory_.size();
  }

  return true;
}

void MessageBroadcaster::onClientConnected(
    const events::ClientConnectedEvent &event) {
  (void)event;
}

void MessageBroadcaster::onClientDisconnected(
    const events::ClientDisconnectedEvent &event) {
  (void)event;
}

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

} // namespace domain
