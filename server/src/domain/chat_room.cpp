#include "domain/chat_room.hpp"

#include <algorithm>
#include <utility>

namespace domain {

ConnectResult ChatRoom::connectClient(const std::string &peer,
                                      const std::string &pseudonym,
                                      const std::string &gender,
                                      const std::string &country) {
  std::string logMessage;
  bool shouldBroadcast = false;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    const bool pseudonymInUse =
        std::any_of(clients_.cbegin(), clients_.cend(),
                    [&pseudonym, &peer](const auto &entry) {
                      return entry.first != peer &&
                             entry.second.pseudonym == pseudonym;
                    });
    if (pseudonymInUse) {
      logMessage = "The pseudo you are using is already in use, please "
                   "choose another one";
      return {false, logMessage};
    }

    ClientInfo info{
        .pseudonym = pseudonym,
        .gender = gender,
        .country = country,
        .next_message_index = message_history_.size(),
        .next_client_event_index = client_events_.size(),
    };

    auto it = clients_.find(peer);
    if (it != clients_.end()) {
      it->second = std::move(info);
      logMessage =
          "Client already registered for peer '" + peer + "' override info.";
    } else {
      clients_.emplace(peer, std::move(info));
      logMessage = "New client '" + pseudonym + "' is now connected";
    }
    shouldBroadcast = true;
  }

  if (shouldBroadcast) {
    broadcastClientEvent(pseudonym, chat::ClientEventData::ADD);
  }

  return {true, logMessage};
}

bool ChatRoom::disconnectClient(const std::string &pseudonym) {
  bool removed = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(clients_.begin(), clients_.end(),
                           [&pseudonym](const auto &entry) {
                             return entry.second.pseudonym == pseudonym;
                           });
    if (it != clients_.end()) {
      clients_.erase(it);
      removed = true;
    }
  }

  if (removed) {
    broadcastClientEvent(pseudonym, chat::ClientEventData::REMOVE);
  }

  return removed;
}

bool ChatRoom::getPseudonymForPeer(const std::string &peer,
                                   std::string *pseudonym) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = clients_.find(peer);
  if (it == clients_.end()) {
    return false;
  }

  if (pseudonym != nullptr) {
    *pseudonym = it->second.pseudonym;
  }
  return true;
}

bool ChatRoom::normalizeMessageIndex(const std::string &peer) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = clients_.find(peer);
  if (it == clients_.end()) {
    return false;
  }

  if (it->second.next_message_index > message_history_.size()) {
    it->second.next_message_index = message_history_.size();
  }
  return true;
}

bool ChatRoom::normalizeClientEventIndex(const std::string &peer) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = clients_.find(peer);
  if (it == clients_.end()) {
    return false;
  }

  if (it->second.next_client_event_index > client_events_.size()) {
    it->second.next_client_event_index = client_events_.size();
  }
  return true;
}

void ChatRoom::addMessage(const std::string &author,
                          const std::string &content) {
  chat::InformClientsNewMessageResponse payload;
  payload.set_author(author);
  payload.set_content(content);

  {
    std::lock_guard<std::mutex> lock(mutex_);
    message_history_.push_back(payload);
  }

  message_cv_.notify_all();
}

NextMessageStatus
ChatRoom::nextMessage(const std::string &peer,
                      std::chrono::milliseconds waitFor,
                      chat::InformClientsNewMessageResponse *out) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto it = clients_.find(peer);
  if (it == clients_.end()) {
    return NextMessageStatus::kPeerMissing;
  }

  if (it->second.next_message_index < message_history_.size()) {
    if (out != nullptr) {
      *out = message_history_[it->second.next_message_index];
    }
    ++it->second.next_message_index;
    return NextMessageStatus::kOk;
  }

  message_cv_.wait_for(lock, waitFor);

  it = clients_.find(peer);
  if (it == clients_.end()) {
    return NextMessageStatus::kPeerMissing;
  }

  if (it->second.next_message_index < message_history_.size()) {
    if (out != nullptr) {
      *out = message_history_[it->second.next_message_index];
    }
    ++it->second.next_message_index;
    return NextMessageStatus::kOk;
  }

  return NextMessageStatus::kNoMessage;
}

bool ChatRoom::getInitialRoster(const std::string &peer,
                                std::vector<std::string> *out) {
  if (out == nullptr) {
    return false;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  auto it = clients_.find(peer);
  if (it == clients_.end()) {
    return false;
  }

  if (it->second.next_client_event_index > client_events_.size()) {
    it->second.next_client_event_index = client_events_.size();
  }

  out->clear();
  out->reserve(clients_.size());
  for (const auto &entry : clients_) {
    out->emplace_back(entry.second.pseudonym);
  }
  return true;
}

NextClientEventStatus
ChatRoom::nextClientEvent(const std::string &peer,
                          std::chrono::milliseconds waitFor,
                          chat::ClientEventData *out) {
  std::unique_lock<std::mutex> lock(mutex_);
  auto it = clients_.find(peer);
  if (it == clients_.end()) {
    return NextClientEventStatus::kPeerMissing;
  }

  if (it->second.next_client_event_index < client_events_.size()) {
    if (out != nullptr) {
      *out = client_events_[it->second.next_client_event_index];
    }
    ++it->second.next_client_event_index;
    return NextClientEventStatus::kOk;
  }

  client_event_cv_.wait_for(lock, waitFor);

  it = clients_.find(peer);
  if (it == clients_.end()) {
    return NextClientEventStatus::kPeerMissing;
  }

  if (it->second.next_client_event_index < client_events_.size()) {
    if (out != nullptr) {
      *out = client_events_[it->second.next_client_event_index];
    }
    ++it->second.next_client_event_index;
    return NextClientEventStatus::kOk;
  }

  return NextClientEventStatus::kNoEvent;
}

void ChatRoom::broadcastClientEvent(
    const std::vector<std::string> &pseudonyms,
    chat::ClientEventData_ClientEventType eventType) {
  chat::ClientEventData payload;
  payload.set_event_type(eventType);

  for (const auto &name : pseudonyms) {
    if (!name.empty()) {
      payload.add_pseudonyms(name);
    }
  }

  if (payload.pseudonyms_size() == 0) {
    return;
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    client_events_.push_back(payload);
  }

  client_event_cv_.notify_all();
}

void ChatRoom::broadcastClientEvent(
    const std::string &pseudonym,
    chat::ClientEventData_ClientEventType eventType) {
  broadcastClientEvent(std::vector<std::string>{pseudonym}, eventType);
}

} // namespace domain
