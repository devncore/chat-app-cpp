#include "domain/client_event_broadcaster.hpp"

#include "service/events/chat_service_events.hpp"

namespace domain {

ClientEventBroadcaster::ClientEventBroadcaster(
    const ClientRegistry &clientRegistry)
    : clientRegistry_(clientRegistry) {}

void ClientEventBroadcaster::broadcastClientEvent(
    const std::string &pseudonym,
    chat::ClientEventData_ClientEventType eventType) {
  if (pseudonym.empty()) {
    return;
  }

  chat::ClientEventData payload;
  payload.set_event_type(eventType);
  payload.set_pseudonym(pseudonym);

  {
    std::lock_guard<std::mutex> lock(mutex_);
    clientEvents_.push_back(payload);
  }

  clientEventCv_.notify_all();
}

NextClientEventStatus ClientEventBroadcaster::nextClientEvent(
    const std::string &peer, std::chrono::milliseconds waitFor,
    chat::ClientEventData &out) {
  std::unique_lock<std::mutex> lock(mutex_);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerIndices_.erase(peer);
    return NextClientEventStatus::kPeerMissing;
  }

  auto it = peerIndices_.find(peer);
  if (it == peerIndices_.end()) {
    it = peerIndices_.emplace(peer, clientEvents_.size()).first;
  }

  if (it->second < clientEvents_.size()) {
    out = clientEvents_[it->second];
    ++it->second;
    return NextClientEventStatus::kOk;
  }

  clientEventCv_.wait_for(lock, waitFor);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerIndices_.erase(peer);
    return NextClientEventStatus::kPeerMissing;
  }

  it = peerIndices_.find(peer);
  if (it == peerIndices_.end()) {
    return NextClientEventStatus::kPeerMissing;
  }

  if (it->second < clientEvents_.size()) {
    out = clientEvents_[it->second];
    ++it->second;
    return NextClientEventStatus::kOk;
  }

  return NextClientEventStatus::kNoEvent;
}

bool ClientEventBroadcaster::normalizeClientEventIndex(const std::string &peer) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!clientRegistry_.isPeerConnected(peer)) {
    peerIndices_.erase(peer);
    return false;
  }

  auto it = peerIndices_.find(peer);
  if (it == peerIndices_.end()) {
    peerIndices_[peer] = clientEvents_.size();
    return true;
  }

  if (it->second > clientEvents_.size()) {
    it->second = clientEvents_.size();
  }

  return true;
}

void ClientEventBroadcaster::onClientConnected(
    const events::ClientConnectedEvent &event) {
  broadcastClientEvent(event.pseudonym, chat::ClientEventData::ADD);
}

void ClientEventBroadcaster::onClientDisconnected(
    const events::ClientDisconnectedEvent &event) {
  broadcastClientEvent(event.pseudonym, chat::ClientEventData::REMOVE);
}

void ClientEventBroadcaster::onMessageSent(
    [[maybe_unused]] const events::MessageSentEvent &event) {}

void ClientEventBroadcaster::onPrivateMessageSent(
    [[maybe_unused]] const events::PrivateMessageSentEvent &event) {}

} // namespace domain
