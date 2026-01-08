#include "domain/chat_room.hpp"

#include <algorithm>
#include <utility>

namespace domain {

ConnectResult ChatRoom::ConnectClient(const std::string &peer,
                                      const std::string &pseudonym,
                                      const std::string &gender,
                                      const std::string &country) {
  std::string log_message;
  bool should_broadcast = false;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    const bool pseudonym_in_use =
        std::any_of(clients_.cbegin(), clients_.cend(),
                    [&pseudonym, &peer](const auto &entry) {
                      return entry.first != peer &&
                             entry.second.pseudonym == pseudonym;
                    });
    if (pseudonym_in_use) {
      log_message = "The pseudo you are using is already in use, please "
                    "choose another one";
      return {false, log_message};
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
      log_message = "Client already registered for peer '" + peer +
                    "' override info.";
    } else {
      clients_.emplace(peer, std::move(info));
      log_message = "New client '" + pseudonym + "' is now connected";
    }
    should_broadcast = true;
  }

  if (should_broadcast) {
    BroadcastClientEvent(pseudonym, chat::ClientEventData::ADD);
  }

  return {true, log_message};
}

bool ChatRoom::DisconnectClient(const std::string &pseudonym) {
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
    BroadcastClientEvent(pseudonym, chat::ClientEventData::REMOVE);
  }

  return removed;
}

bool ChatRoom::GetPseudonymForPeer(const std::string &peer,
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

bool ChatRoom::NormalizeMessageIndex(const std::string &peer) {
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

bool ChatRoom::NormalizeClientEventIndex(const std::string &peer) {
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

void ChatRoom::AddMessage(const std::string &author,
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
ChatRoom::NextMessage(const std::string &peer,
                      std::chrono::milliseconds wait_for,
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

  message_cv_.wait_for(lock, wait_for);

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

bool ChatRoom::GetInitialRoster(const std::string &peer,
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
ChatRoom::NextClientEvent(const std::string &peer,
                          std::chrono::milliseconds wait_for,
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

  client_event_cv_.wait_for(lock, wait_for);

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

void ChatRoom::BroadcastClientEvent(
    const std::vector<std::string> &pseudonyms,
    chat::ClientEventData_ClientEventType event_type) {
  chat::ClientEventData payload;
  payload.set_eventtype(event_type);

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

void ChatRoom::BroadcastClientEvent(
    const std::string &pseudonym,
    chat::ClientEventData_ClientEventType event_type) {
  BroadcastClientEvent(std::vector<std::string>{pseudonym}, event_type);
}

} // namespace domain
