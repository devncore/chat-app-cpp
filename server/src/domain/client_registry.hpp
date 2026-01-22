#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "service/events/chat_service_events.hpp"

namespace domain {

struct ClientInfo {
  std::string pseudonym;
  std::string gender;
  std::string country;
  std::chrono::steady_clock::time_point initialTimePoint;
};

class ClientRegistry : public events::IServiceEventObserver {
public:
  bool isPseudonymAvailable(const std::string &peer,
                            const std::string &pseudonym) const;

  bool getPseudonymForPeer(const std::string &peer, std::string &out) const;

  bool getPeerForPseudonym(const std::string &pseudonym, std::string &out) const;

  std::optional<std::chrono::steady_clock::duration>
  getConnectionDuration(const std::string &peer) const;

  std::vector<std::string> getConnectedPseudonyms() const;

  bool isPeerConnected(const std::string &peer) const;

  // Grant access to IServiceEventObserver interface for registration
  events::IServiceEventObserver *asObserver();

private:
  void onClientConnected(const events::ClientConnectedEvent &event) override;
  void
  onClientDisconnected(const events::ClientDisconnectedEvent &event) override;
  void onMessageSent(const events::MessageSentEvent &event) override;
  void onPrivateMessageSent(const events::PrivateMessageSentEvent &event) override;

  mutable std::mutex mutex_;
  std::unordered_map<std::string, ClientInfo> clients_;
};

} // namespace domain
