#include "domain/private_message_broadcaster.hpp"

namespace domain {

PrivateMessageBroadcaster::PrivateMessageBroadcaster(
    const ClientRegistry &clientRegistry)
    : clientRegistry_(clientRegistry) {}

NextPrivateMessageStatus PrivateMessageBroadcaster::nextPrivateMessage(
    const std::string &peer, std::chrono::milliseconds waitFor,
    chat::InformClientsNewMessageResponse &out) {
  std::unique_lock<std::mutex> lock(mutex_);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerMessageQueues_.erase(peer);
    return NextPrivateMessageStatus::kPeerMissing;
  }

  auto &queue = peerMessageQueues_[peer];

  if (!queue.empty()) {
    out = std::move(queue.front());
    queue.pop_front();
    return NextPrivateMessageStatus::kOk;
  }

  messageCv_.wait_for(lock, waitFor);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerMessageQueues_.erase(peer);
    return NextPrivateMessageStatus::kPeerMissing;
  }

  auto queueIt = peerMessageQueues_.find(peer);
  if (queueIt == peerMessageQueues_.end() || queueIt->second.empty()) {
    return NextPrivateMessageStatus::kNoMessage;
  }

  out = std::move(queueIt->second.front());
  queueIt->second.pop_front();
  return NextPrivateMessageStatus::kOk;
}

bool PrivateMessageBroadcaster::normalizePrivateMessageIndex(
    const std::string &peer) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerMessageQueues_.erase(peer);
    return false;
  }

  peerMessageQueues_.try_emplace(peer);

  return true;
}

void PrivateMessageBroadcaster::onClientConnected(
    [[maybe_unused]] const events::ClientConnectedEvent &event) {}

void PrivateMessageBroadcaster::onClientDisconnected(
    [[maybe_unused]] const events::ClientDisconnectedEvent &event) {
  std::lock_guard<std::mutex> lock(mutex_);
  // Clean up message queue for disconnected peer
  for (auto it = peerMessageQueues_.begin(); it != peerMessageQueues_.end();) {
    if (!clientRegistry_.isPeerConnected(it->first)) {
      it = peerMessageQueues_.erase(it);
    } else {
      ++it;
    }
  }
}

void PrivateMessageBroadcaster::onMessageSent(
    [[maybe_unused]] const events::MessageSentEvent &event) {}

void PrivateMessageBroadcaster::onPrivateMessageSent(
    const events::PrivateMessageSentEvent &event) {
  chat::InformClientsNewMessageResponse payload;
  payload.set_author(event.senderPseudonym);
  payload.set_content(event.content);
  payload.set_isprivate(true);

  {
    std::lock_guard<std::mutex> lock(mutex_);
    // Add message to recipient's queue
    peerMessageQueues_[event.recipientPeer].push_back(payload);
  }

  messageCv_.notify_all();
}

} // namespace domain
