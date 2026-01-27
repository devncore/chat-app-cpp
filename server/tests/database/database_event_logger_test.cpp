#include <gtest/gtest.h>

#include "database/database_event_logger.hpp"
#include "mock/mock_database_manager.hpp"

#include <chrono>
#include <memory>

namespace observers {
namespace {

class DatabaseEventLoggerTest : public ::testing::Test {
protected:
  std::shared_ptr<mock::MockDatabaseManager> mockDb_;
  std::unique_ptr<DatabaseEventLogger> logger_;

  void SetUp() override {
    mockDb_ = std::make_shared<mock::MockDatabaseManager>();
    logger_ = std::make_unique<DatabaseEventLogger>(mockDb_);
  }

  void TearDown() override {
    // Reset mock state
    if (mockDb_) {
      mockDb_->reset();
    }
  }
};

// --- onClientConnected Tests ---

TEST_F(DatabaseEventLoggerTest,
       OnClientConnected_CallsClientConnectionEvent) {
  events::ClientConnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .gender = "female",
      .country = "US",
  };

  logger_->onClientConnected(event);

  EXPECT_EQ(mockDb_->clientConnectionEventCalls, 1);
  EXPECT_EQ(mockDb_->lastPseudonym, "alice");
}

TEST_F(DatabaseEventLoggerTest,
       OnClientConnected_WithDifferentPseudonyms) {
  events::ClientConnectedEvent event1{
      .peer = "peer1",
      .pseudonym = "alice",
      .gender = "female",
      .country = "US",
  };
  events::ClientConnectedEvent event2{
      .peer = "peer2",
      .pseudonym = "bob",
      .gender = "male",
      .country = "UK",
  };

  logger_->onClientConnected(event1);
  logger_->onClientConnected(event2);

  EXPECT_EQ(mockDb_->clientConnectionEventCalls, 2);
  EXPECT_EQ(mockDb_->lastPseudonym, "bob");
}

TEST_F(DatabaseEventLoggerTest,
       OnClientConnected_DatabaseError_DoesNotThrow) {
  mockDb_->clientConnectionEventFn = [](std::string_view) {
    return std::optional<std::string>("Database error");
  };

  events::ClientConnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .gender = "female",
      .country = "US",
  };

  // Should not throw, just log error to stderr
  EXPECT_NO_THROW(logger_->onClientConnected(event));
  EXPECT_EQ(mockDb_->clientConnectionEventCalls, 1);
}

TEST_F(DatabaseEventLoggerTest,
       OnClientConnected_DatabaseUnavailable_DoesNotThrow) {
  // Create logger with expired weak_ptr
  std::weak_ptr<database::IDatabaseManager> expiredDb;
  {
    auto tempDb = std::make_shared<mock::MockDatabaseManager>();
    expiredDb = tempDb;
  }
  auto loggerWithExpiredDb =
      std::make_unique<DatabaseEventLogger>(expiredDb);

  events::ClientConnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .gender = "female",
      .country = "US",
  };

  // Should not throw or crash when database is unavailable
  EXPECT_NO_THROW(loggerWithExpiredDb->onClientConnected(event));
}

// --- onClientDisconnected Tests ---

TEST_F(DatabaseEventLoggerTest,
       OnClientDisconnected_CallsUpdateCumulatedConnectionTime) {
  events::ClientDisconnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .connectionDuration = std::chrono::seconds(120),
  };

  logger_->onClientDisconnected(event);

  EXPECT_EQ(mockDb_->updateCumulatedConnectionTimeCalls, 1);
  EXPECT_EQ(mockDb_->lastPseudonym, "alice");
  EXPECT_EQ(mockDb_->lastDurationInSec, 120);
}

TEST_F(DatabaseEventLoggerTest,
       OnClientDisconnected_ZeroDuration) {
  events::ClientDisconnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .connectionDuration = std::chrono::seconds(0),
  };

  logger_->onClientDisconnected(event);

  EXPECT_EQ(mockDb_->updateCumulatedConnectionTimeCalls, 1);
  EXPECT_EQ(mockDb_->lastDurationInSec, 0);
}

TEST_F(DatabaseEventLoggerTest,
       OnClientDisconnected_LargeDuration) {
  events::ClientDisconnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .connectionDuration = std::chrono::hours(24), // 86400 seconds
  };

  logger_->onClientDisconnected(event);

  EXPECT_EQ(mockDb_->updateCumulatedConnectionTimeCalls, 1);
  EXPECT_EQ(mockDb_->lastDurationInSec, 86400);
}

TEST_F(DatabaseEventLoggerTest,
       OnClientDisconnected_SubSecondDuration_TruncatesToSeconds) {
  events::ClientDisconnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .connectionDuration = std::chrono::milliseconds(1500), // 1.5 seconds
  };

  logger_->onClientDisconnected(event);

  EXPECT_EQ(mockDb_->updateCumulatedConnectionTimeCalls, 1);
  // Should be truncated to 1 second
  EXPECT_EQ(mockDb_->lastDurationInSec, 1);
}

TEST_F(DatabaseEventLoggerTest,
       OnClientDisconnected_DatabaseError_DoesNotThrow) {
  mockDb_->updateCumulatedConnectionTimeFn =
      [](std::string_view, uint64_t) {
        return std::optional<std::string>("Database error");
      };

  events::ClientDisconnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .connectionDuration = std::chrono::seconds(60),
  };

  EXPECT_NO_THROW(logger_->onClientDisconnected(event));
  EXPECT_EQ(mockDb_->updateCumulatedConnectionTimeCalls, 1);
}

TEST_F(DatabaseEventLoggerTest,
       OnClientDisconnected_DatabaseUnavailable_DoesNotThrow) {
  std::weak_ptr<database::IDatabaseManager> expiredDb;
  {
    auto tempDb = std::make_shared<mock::MockDatabaseManager>();
    expiredDb = tempDb;
  }
  auto loggerWithExpiredDb =
      std::make_unique<DatabaseEventLogger>(expiredDb);

  events::ClientDisconnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .connectionDuration = std::chrono::seconds(60),
  };

  EXPECT_NO_THROW(loggerWithExpiredDb->onClientDisconnected(event));
}

// --- onMessageSent Tests ---

TEST_F(DatabaseEventLoggerTest, OnMessageSent_CallsIncrementTxMessage) {
  events::MessageSentEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .content = "Hello, World!",
  };

  logger_->onMessageSent(event);

  EXPECT_EQ(mockDb_->incrementTxMessageCalls, 1);
  EXPECT_EQ(mockDb_->lastPseudonym, "alice");
}

TEST_F(DatabaseEventLoggerTest,
       OnMessageSent_MultipleMessages_IncrementsForEach) {
  events::MessageSentEvent event1{
      .peer = "peer1",
      .pseudonym = "alice",
      .content = "Message 1",
  };
  events::MessageSentEvent event2{
      .peer = "peer1",
      .pseudonym = "alice",
      .content = "Message 2",
  };
  events::MessageSentEvent event3{
      .peer = "peer2",
      .pseudonym = "bob",
      .content = "Message 3",
  };

  logger_->onMessageSent(event1);
  logger_->onMessageSent(event2);
  logger_->onMessageSent(event3);

  EXPECT_EQ(mockDb_->incrementTxMessageCalls, 3);
  EXPECT_EQ(mockDb_->lastPseudonym, "bob");
}

TEST_F(DatabaseEventLoggerTest, OnMessageSent_DatabaseError_DoesNotThrow) {
  mockDb_->incrementTxMessageFn = [](std::string_view) {
    return std::optional<std::string>("Database error");
  };

  events::MessageSentEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .content = "Hello",
  };

  EXPECT_NO_THROW(logger_->onMessageSent(event));
  EXPECT_EQ(mockDb_->incrementTxMessageCalls, 1);
}

TEST_F(DatabaseEventLoggerTest,
       OnMessageSent_DatabaseUnavailable_DoesNotThrow) {
  std::weak_ptr<database::IDatabaseManager> expiredDb;
  {
    auto tempDb = std::make_shared<mock::MockDatabaseManager>();
    expiredDb = tempDb;
  }
  auto loggerWithExpiredDb =
      std::make_unique<DatabaseEventLogger>(expiredDb);

  events::MessageSentEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .content = "Hello",
  };

  EXPECT_NO_THROW(loggerWithExpiredDb->onMessageSent(event));
}

// --- Integration-style Tests ---

TEST_F(DatabaseEventLoggerTest, FullLifecycle_ConnectMessageDisconnect) {
  // Client connects
  events::ClientConnectedEvent connectEvent{
      .peer = "peer1",
      .pseudonym = "alice",
      .gender = "female",
      .country = "US",
  };
  logger_->onClientConnected(connectEvent);

  // Client sends messages
  events::MessageSentEvent msg1{
      .peer = "peer1",
      .pseudonym = "alice",
      .content = "Hello",
  };
  events::MessageSentEvent msg2{
      .peer = "peer1",
      .pseudonym = "alice",
      .content = "Goodbye",
  };
  logger_->onMessageSent(msg1);
  logger_->onMessageSent(msg2);

  // Client disconnects
  events::ClientDisconnectedEvent disconnectEvent{
      .peer = "peer1",
      .pseudonym = "alice",
      .connectionDuration = std::chrono::minutes(5), // 300 seconds
  };
  logger_->onClientDisconnected(disconnectEvent);

  // Verify all database calls were made
  EXPECT_EQ(mockDb_->clientConnectionEventCalls, 1);
  EXPECT_EQ(mockDb_->incrementTxMessageCalls, 2);
  EXPECT_EQ(mockDb_->updateCumulatedConnectionTimeCalls, 1);
  EXPECT_EQ(mockDb_->lastDurationInSec, 300);
}

} // namespace
} // namespace observers
