#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
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
  bool isPseudonymAvailable(std::string_view peer,
                            std::string_view pseudonym) const;

  bool getPseudonymForPeer(std::string_view peer, std::string &out) const;

  bool getPeerForPseudonym(std::string_view pseudonym, std::string &out) const;

  std::optional<std::chrono::steady_clock::duration>
  getConnectionDuration(std::string_view peer) const;

  std::vector<std::string> getConnectedPseudonyms() const;

  bool isPeerConnected(std::string_view peer) const;

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
