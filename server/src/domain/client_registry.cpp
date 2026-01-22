#include "domain/client_registry.hpp"

#include <algorithm>

#include "service/events/chat_service_events.hpp"

namespace domain {

bool ClientRegistry::isPseudonymAvailable(const std::string &peer,
                                          const std::string &pseudonym) const {
  std::lock_guard<std::mutex> lock(mutex_);

  return !std::any_of(
      clients_.cbegin(), clients_.cend(),
      [&pseudonym, &peer](const auto &entry) {
        return entry.first != peer && entry.second.pseudonym == pseudonym;
      });
}

bool ClientRegistry::getPseudonymForPeer(const std::string &peer,
                                         std::string &out) const {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = clients_.find(peer);
  if (it == clients_.end()) {
    return false;
  }

  out = it->second.pseudonym;
  return true;
}

bool ClientRegistry::getPeerForPseudonym(const std::string &pseudonym,
                                         std::string &out) const {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = std::find_if(clients_.cbegin(), clients_.cend(),
                         [&pseudonym](const auto &entry) {
                           return entry.second.pseudonym == pseudonym;
                         });

  if (it == clients_.end()) {
    return false;
  }

  out = it->first;
  return true;
}

std::optional<std::chrono::steady_clock::duration>
ClientRegistry::getConnectionDuration(const std::string &peer) const {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = clients_.find(peer);
  if (it == clients_.end()) {
    return std::nullopt;
  }

  return std::chrono::steady_clock::now() - it->second.initialTimePoint;
}

std::vector<std::string> ClientRegistry::getConnectedPseudonyms() const {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<std::string> pseudonyms;
  pseudonyms.reserve(clients_.size());

  for (const auto &entry : clients_) {
    pseudonyms.emplace_back(entry.second.pseudonym);
  }

  return pseudonyms;
}

bool ClientRegistry::isPeerConnected(const std::string &peer) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return clients_.find(peer) != clients_.end();
}

events::IServiceEventObserver *ClientRegistry::asObserver() { return this; }

void ClientRegistry::onClientConnected(
    const events::ClientConnectedEvent &event) {
  std::lock_guard<std::mutex> lock(mutex_);

  ClientInfo info{
      .pseudonym = event.pseudonym,
      .gender = event.gender,
      .country = event.country,
      .initialTimePoint = std::chrono::steady_clock::now(),
  };

  clients_[event.peer] = std::move(info);
}

void ClientRegistry::onClientDisconnected(
    const events::ClientDisconnectedEvent &event) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = std::find_if(clients_.begin(), clients_.end(),
                         [&event](const auto &entry) {
                           return entry.second.pseudonym == event.pseudonym;
                         });

  if (it != clients_.end()) {
    clients_.erase(it);
  }
}

void ClientRegistry::onMessageSent(
    [[maybe_unused]] const events::MessageSentEvent &event) {}

void ClientRegistry::onPrivateMessageSent(
    [[maybe_unused]] const events::PrivateMessageSentEvent &event) {}

} // namespace domain
