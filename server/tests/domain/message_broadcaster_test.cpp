#include <gtest/gtest.h>

#include "domain/client_registry.hpp"
#include "domain/message_broadcaster.hpp"

#include <chrono>
#include <thread>

namespace domain {
namespace {

class MessageBroadcasterTest : public ::testing::Test {
protected:
  ClientRegistry registry_;
  std::unique_ptr<MessageBroadcaster> broadcaster_;

  void SetUp() override {
    broadcaster_ = std::make_unique<MessageBroadcaster>(registry_);
  }

  void connectClient(const std::string &peer, const std::string &pseudonym,
                     const std::string &gender = "male",
                     const std::string &country = "US") {
    events::ClientConnectedEvent event{
        .peer = peer,
        .pseudonym = pseudonym,
        .gender = gender,
        .country = country,
    };
    registry_.asObserver()->onClientConnected(event);
  }

  void disconnectClient(const std::string &pseudonym) {
    events::ClientDisconnectedEvent event{
        .peer = "",
        .pseudonym = pseudonym,
        .connectionDuration = std::chrono::seconds(0),
    };
    registry_.asObserver()->onClientDisconnected(event);
  }

  void sendMessage(const std::string &peer, const std::string &pseudonym,
                   const std::string &content) {
    events::MessageSentEvent event{
        .peer = peer,
        .pseudonym = pseudonym,
        .content = content,
    };
    broadcaster_->onMessageSent(event);
  }
};

// --- nextMessage Tests ---

TEST_F(MessageBroadcasterTest, NextMessage_PeerNotConnected_ReturnsPeerMissing) {
  auto status = broadcaster_->nextMessage("unknown_peer",
                                          std::chrono::milliseconds(0), nullptr);
  EXPECT_EQ(status, NextMessageStatus::kPeerMissing);
}

TEST_F(MessageBroadcasterTest, NextMessage_NoMessages_ReturnsNoMessage) {
  connectClient("peer1", "alice");

  auto status =
      broadcaster_->nextMessage("peer1", std::chrono::milliseconds(10), nullptr);
  EXPECT_EQ(status, NextMessageStatus::kNoMessage);
}

TEST_F(MessageBroadcasterTest, NextMessage_HasMessage_ReturnsOk) {
  connectClient("peer1", "alice");

  // Initialize peer's index first by calling nextMessage (will return NoMessage)
  broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), nullptr);

  // Now send message - peer will see it
  sendMessage("peer1", "alice", "Hello!");

  chat::InformClientsNewMessageResponse response;
  auto status =
      broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), &response);

  EXPECT_EQ(status, NextMessageStatus::kOk);
  EXPECT_EQ(response.author(), "alice");
  EXPECT_EQ(response.content(), "Hello!");
}

TEST_F(MessageBroadcasterTest, NextMessage_MultipleMessages_ReturnsInOrder) {
  connectClient("peer1", "alice");

  // Initialize peer's index first
  broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), nullptr);

  sendMessage("peer1", "alice", "First");
  sendMessage("peer1", "alice", "Second");
  sendMessage("peer1", "alice", "Third");

  chat::InformClientsNewMessageResponse response;

  auto status1 =
      broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), &response);
  EXPECT_EQ(status1, NextMessageStatus::kOk);
  EXPECT_EQ(response.content(), "First");

  auto status2 =
      broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), &response);
  EXPECT_EQ(status2, NextMessageStatus::kOk);
  EXPECT_EQ(response.content(), "Second");

  auto status3 =
      broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), &response);
  EXPECT_EQ(status3, NextMessageStatus::kOk);
  EXPECT_EQ(response.content(), "Third");
}

TEST_F(MessageBroadcasterTest, NextMessage_NullOutput_StillReturnsOk) {
  connectClient("peer1", "alice");

  // Initialize peer's index first
  broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), nullptr);

  sendMessage("peer1", "alice", "Hello!");

  auto status =
      broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), nullptr);
  EXPECT_EQ(status, NextMessageStatus::kOk);
}

TEST_F(MessageBroadcasterTest, NextMessage_MultiplePeers_IndependentIndices) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");

  // Initialize both peers' indices first
  broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), nullptr);
  broadcaster_->nextMessage("peer2", std::chrono::milliseconds(0), nullptr);

  sendMessage("peer1", "alice", "Message1");
  sendMessage("peer2", "bob", "Message2");

  chat::InformClientsNewMessageResponse response1, response2;

  // peer1 should see Message1 first
  auto status1 =
      broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), &response1);
  EXPECT_EQ(status1, NextMessageStatus::kOk);
  EXPECT_EQ(response1.content(), "Message1");

  // peer2 should also see Message1 first (independent index)
  auto status2 =
      broadcaster_->nextMessage("peer2", std::chrono::milliseconds(0), &response2);
  EXPECT_EQ(status2, NextMessageStatus::kOk);
  EXPECT_EQ(response2.content(), "Message1");
}

TEST_F(MessageBroadcasterTest,
       NextMessage_PeerDisconnectedDuringWait_ReturnsPeerMissing) {
  connectClient("peer1", "alice");

  std::thread disconnectThread([this]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    disconnectClient("alice");
  });

  auto status = broadcaster_->nextMessage("peer1", std::chrono::milliseconds(100),
                                          nullptr);
  disconnectThread.join();

  // After timeout, peer is disconnected
  EXPECT_EQ(status, NextMessageStatus::kPeerMissing);
}

TEST_F(MessageBroadcasterTest,
       NextMessage_NewPeer_StartsAtCurrentHistoryPosition) {
  connectClient("peer1", "alice");
  sendMessage("peer1", "alice", "Old message");

  // New peer connects after message was sent
  connectClient("peer2", "bob");

  // peer2 should NOT see the old message (starts at current position)
  auto status =
      broadcaster_->nextMessage("peer2", std::chrono::milliseconds(10), nullptr);
  EXPECT_EQ(status, NextMessageStatus::kNoMessage);
}

// --- normalizeMessageIndex Tests ---

TEST_F(MessageBroadcasterTest,
       NormalizeMessageIndex_PeerNotConnected_ReturnsFalse) {
  bool result = broadcaster_->normalizeMessageIndex("unknown_peer");
  EXPECT_FALSE(result);
}

TEST_F(MessageBroadcasterTest,
       NormalizeMessageIndex_ConnectedPeer_ReturnsTrue) {
  connectClient("peer1", "alice");

  bool result = broadcaster_->normalizeMessageIndex("peer1");
  EXPECT_TRUE(result);
}

TEST_F(MessageBroadcasterTest,
       NormalizeMessageIndex_NewPeer_InitializesIndex) {
  connectClient("peer1", "alice");
  sendMessage("peer1", "alice", "Message1");
  sendMessage("peer1", "alice", "Message2");

  // Normalize for a peer that hasn't requested messages yet
  bool result = broadcaster_->normalizeMessageIndex("peer1");
  EXPECT_TRUE(result);

  // After normalize, peer should start at current position (no old messages)
  auto status =
      broadcaster_->nextMessage("peer1", std::chrono::milliseconds(10), nullptr);
  EXPECT_EQ(status, NextMessageStatus::kNoMessage);
}

// --- onMessageSent Tests ---

TEST_F(MessageBroadcasterTest, OnMessageSent_AddsMessageToHistory) {
  connectClient("peer1", "alice");

  // Initialize peer's index first
  broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), nullptr);

  events::MessageSentEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .content = "Test content",
  };
  broadcaster_->onMessageSent(event);

  chat::InformClientsNewMessageResponse response;
  auto status =
      broadcaster_->nextMessage("peer1", std::chrono::milliseconds(0), &response);

  EXPECT_EQ(status, NextMessageStatus::kOk);
  EXPECT_EQ(response.author(), "alice");
  EXPECT_EQ(response.content(), "Test content");
}

TEST_F(MessageBroadcasterTest, OnMessageSent_WakesWaitingPeers) {
  connectClient("peer1", "alice");

  std::atomic<NextMessageStatus> status{NextMessageStatus::kNoMessage};
  std::atomic<bool> messageReceived{false};

  std::thread waitingThread([this, &status, &messageReceived]() {
    chat::InformClientsNewMessageResponse response;
    status = broadcaster_->nextMessage("peer1", std::chrono::milliseconds(500),
                                       &response);
    if (status == NextMessageStatus::kOk) {
      messageReceived = true;
    }
  });

  // Give the thread time to start waiting
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Send a message which should wake up the waiting thread
  sendMessage("peer1", "alice", "Wake up!");

  waitingThread.join();

  EXPECT_EQ(status.load(), NextMessageStatus::kOk);
  EXPECT_TRUE(messageReceived.load());
}

// --- Observer interface Tests ---

TEST_F(MessageBroadcasterTest, OnClientConnected_NoOp) {
  events::ClientConnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .gender = "female",
      .country = "US",
  };

  // Should not crash
  EXPECT_NO_THROW(broadcaster_->onClientConnected(event));
}

TEST_F(MessageBroadcasterTest, OnClientDisconnected_NoOp) {
  events::ClientDisconnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .connectionDuration = std::chrono::seconds(60),
  };

  // Should not crash
  EXPECT_NO_THROW(broadcaster_->onClientDisconnected(event));
}

} // namespace
} // namespace domain
