#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "chat.pb.h"

namespace domain {

struct ClientInfo {
  std::string pseudonym;
  std::string gender;
  std::string country;
  std::size_t next_message_index{0};
  std::size_t next_client_event_index{0};
};

struct ConnectResult {
  bool accepted{false};
  std::string message;
};

enum class NextMessageStatus {
  kOk,
  kNoMessage,
  kPeerMissing,
};

enum class NextClientEventStatus {
  kOk,
  kNoEvent,
  kPeerMissing,
};

class ChatRoom {
public:
  ConnectResult ConnectClient(const std::string &peer,
                              const std::string &pseudonym,
                              const std::string &gender,
                              const std::string &country);

  bool DisconnectClient(const std::string &pseudonym);

  bool GetPseudonymForPeer(const std::string &peer, std::string *pseudonym);

  bool NormalizeMessageIndex(const std::string &peer);
  bool NormalizeClientEventIndex(const std::string &peer);

  void AddMessage(const std::string &author, const std::string &content);

  NextMessageStatus NextMessage(
      const std::string &peer, std::chrono::milliseconds wait_for,
      chat::InformClientsNewMessageResponse *out);

  bool GetInitialRoster(const std::string &peer,
                        std::vector<std::string> *out);

  NextClientEventStatus
  NextClientEvent(const std::string &peer, std::chrono::milliseconds wait_for,
                  chat::ClientEventData *out);

private:
  void BroadcastClientEvent(const std::vector<std::string> &pseudonyms,
                            chat::ClientEventData_ClientEventType event_type);
  void BroadcastClientEvent(const std::string &pseudonym,
                            chat::ClientEventData_ClientEventType event_type);

  std::mutex mutex_;
  std::condition_variable message_cv_;
  std::vector<chat::InformClientsNewMessageResponse> message_history_;
  std::condition_variable client_event_cv_;
  std::vector<chat::ClientEventData> client_events_;
  std::unordered_map<std::string, ClientInfo> clients_;
};

} // namespace domain
