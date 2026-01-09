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
  ConnectResult connectClient(const std::string &peer,
                              const std::string &pseudonym,
                              const std::string &gender,
                              const std::string &country);

  bool disconnectClient(const std::string &pseudonym);

  bool getPseudonymForPeer(const std::string &peer, std::string *pseudonym);

  bool normalizeMessageIndex(const std::string &peer);
  bool normalizeClientEventIndex(const std::string &peer);

  void addMessage(const std::string &author, const std::string &content);

  NextMessageStatus nextMessage(
      const std::string &peer, std::chrono::milliseconds waitFor,
      chat::InformClientsNewMessageResponse *out);

  bool getInitialRoster(const std::string &peer,
                        std::vector<std::string> *out);

  NextClientEventStatus
  nextClientEvent(const std::string &peer, std::chrono::milliseconds waitFor,
                  chat::ClientEventData *out);

private:
  void broadcastClientEvent(const std::vector<std::string> &pseudonyms,
                            chat::ClientEventData_ClientEventType eventType);
  void broadcastClientEvent(const std::string &pseudonym,
                            chat::ClientEventData_ClientEventType eventType);

  std::mutex mutex_;
  std::condition_variable message_cv_;
  std::vector<chat::InformClientsNewMessageResponse> message_history_;
  std::condition_variable client_event_cv_;
  std::vector<chat::ClientEventData> client_events_;
  std::unordered_map<std::string, ClientInfo> clients_;
};

} // namespace domain
