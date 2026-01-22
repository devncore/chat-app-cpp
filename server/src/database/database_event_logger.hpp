#pragma once

#include "database/database_manager.hpp"
#include "service/events/chat_service_events.hpp"

#include <chrono>
#include <iostream>
#include <memory>

#define DB_LOG_GET_OR_RETURN(var)                                              \
  auto var = getDatabase();                                                    \
  if (!var) {                                                                  \
    std::cerr << "Database unavailable in DatabaseEventLogger::" << __func__   \
              << std::endl;                                                    \
    return;                                                                    \
  }

namespace observers {

class DatabaseEventLogger : public events::IServiceEventObserver {
public:
  explicit DatabaseEventLogger(std::weak_ptr<database::IDatabaseManager> db)
      : db_(std::move(db)) {}

  void onClientConnected(const events::ClientConnectedEvent &event) override {
    DB_LOG_GET_OR_RETURN(db);

    if (auto error = db->clientConnectionEvent(event.pseudonym)) {
      std::cerr << "Database error on connection: " << *error << std::endl;
    }
  }

  void
  onClientDisconnected(const events::ClientDisconnectedEvent &event) override {
    DB_LOG_GET_OR_RETURN(db);

    const auto durationSec =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                  event.connectionDuration)
                                  .count());

    if (auto error =
            db->updateCumulatedConnectionTime(event.pseudonym, durationSec)) {
      std::cerr << "Database error on disconnect: " << *error << std::endl;
    }
  }

  void onMessageSent(const events::MessageSentEvent &event) override {
    DB_LOG_GET_OR_RETURN(db);

    if (auto error = db->incrementTxMessage(event.pseudonym)) {
      std::cerr << "Database error on message sent: " << *error << std::endl;
    }
  }

  void onPrivateMessageSent(const events::PrivateMessageSentEvent &event) override {
    DB_LOG_GET_OR_RETURN(db);

    if (auto error = db->incrementTxMessage(event.senderPseudonym)) {
      std::cerr << "Database error on private message sent: " << *error << std::endl;
    }
  }

private:
  std::shared_ptr<database::IDatabaseManager> getDatabase() {
    return db_.lock();
  }

  std::weak_ptr<database::IDatabaseManager> db_;
};

} // namespace observers
