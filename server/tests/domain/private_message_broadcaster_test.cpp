#include <gtest/gtest.h>

#include "domain/client_registry.hpp"
#include "domain/private_message_broadcaster.hpp"

#include <chrono>
#include <thread>

namespace domain {
namespace {

class PrivateMessageBroadcasterTest : public ::testing::Test {
protected:
  ClientRegistry registry_;
  std::unique_ptr<PrivateMessageBroadcaster> broadcaster_;

  void SetUp() override {
    broadcaster_ = std::make_unique<PrivateMessageBroadcaster>(registry_);
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

  void sendPrivateMessage(const std::string &senderPeer,
                          const std::string &senderPseudonym,
                          const std::string &recipientPeer,
                          const std::string &recipientPseudonym,
                          const std::string &content) {
    events::PrivateMessageSentEvent event{
        .senderPeer = senderPeer,
        .senderPseudonym = senderPseudonym,
        .recipientPeer = recipientPeer,
        .recipientPseudonym = recipientPseudonym,
        .content = content,
    };
    broadcaster_->onPrivateMessageSent(event);
  }
};

// --- nextPrivateMessage Tests ---

TEST_F(PrivateMessageBroadcasterTest,
       NextPrivateMessage_PeerNotConnected_ReturnsPeerMissing) {
  chat::InformClientsNewMessageResponse response;
  auto status = broadcaster_->nextPrivateMessage(
      "unknown_peer", std::chrono::milliseconds(0), response);
  EXPECT_EQ(status, NextPrivateMessageStatus::kPeerMissing);
}

TEST_F(PrivateMessageBroadcasterTest,
       NextPrivateMessage_NoMessages_ReturnsNoMessage) {
  connectClient("peer1", "alice");

  chat::InformClientsNewMessageResponse response;
  auto status = broadcaster_->nextPrivateMessage(
      "peer1", std::chrono::milliseconds(10), response);
  EXPECT_EQ(status, NextPrivateMessageStatus::kNoMessage);
}

TEST_F(PrivateMessageBroadcasterTest, NextPrivateMessage_HasMessage_ReturnsOk) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");

  // Initialize peer's queue
  broadcaster_->normalizePrivateMessageIndex("peer2");

  // Send private message from alice to bob
  sendPrivateMessage("peer1", "alice", "peer2", "bob", "Hello Bob!");

  chat::InformClientsNewMessageResponse response;
  auto status = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(0), response);

  EXPECT_EQ(status, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(response.author(), "alice");
  EXPECT_EQ(response.content(), "Hello Bob!");
  EXPECT_TRUE(response.isprivate());
}

TEST_F(PrivateMessageBroadcasterTest,
       NextPrivateMessage_MultipleMessages_ReturnsInOrder) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");

  broadcaster_->normalizePrivateMessageIndex("peer2");

  sendPrivateMessage("peer1", "alice", "peer2", "bob", "First");
  sendPrivateMessage("peer1", "alice", "peer2", "bob", "Second");
  sendPrivateMessage("peer1", "alice", "peer2", "bob", "Third");

  chat::InformClientsNewMessageResponse response;

  auto status1 = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(0), response);
  EXPECT_EQ(status1, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(response.content(), "First");

  auto status2 = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(0), response);
  EXPECT_EQ(status2, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(response.content(), "Second");

  auto status3 = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(0), response);
  EXPECT_EQ(status3, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(response.content(), "Third");
}

TEST_F(PrivateMessageBroadcasterTest,
       NextPrivateMessage_OnlyRecipientReceives) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");
  connectClient("peer3", "charlie");

  broadcaster_->normalizePrivateMessageIndex("peer2");
  broadcaster_->normalizePrivateMessageIndex("peer3");

  // Send private message from alice to bob only
  sendPrivateMessage("peer1", "alice", "peer2", "bob", "Secret for Bob");

  // Bob should receive the message
  chat::InformClientsNewMessageResponse bobResponse;
  auto bobStatus = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(0), bobResponse);
  EXPECT_EQ(bobStatus, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(bobResponse.content(), "Secret for Bob");

  // Charlie should NOT receive the message
  chat::InformClientsNewMessageResponse charlieResponse;
  auto charlieStatus = broadcaster_->nextPrivateMessage(
      "peer3", std::chrono::milliseconds(10), charlieResponse);
  EXPECT_EQ(charlieStatus, NextPrivateMessageStatus::kNoMessage);
}

TEST_F(PrivateMessageBroadcasterTest,
       NextPrivateMessage_PeerDisconnectedDuringWait_ReturnsPeerMissing) {
  connectClient("peer1", "alice");

  std::thread disconnectThread([this]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    disconnectClient("alice");
  });

  chat::InformClientsNewMessageResponse response;
  auto status = broadcaster_->nextPrivateMessage(
      "peer1", std::chrono::milliseconds(100), response);
  disconnectThread.join();

  EXPECT_EQ(status, NextPrivateMessageStatus::kPeerMissing);
}

TEST_F(PrivateMessageBroadcasterTest,
       NextPrivateMessage_MessagesFromMultipleSenders) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");
  connectClient("peer3", "charlie");

  broadcaster_->normalizePrivateMessageIndex("peer1");

  // Both bob and charlie send messages to alice
  sendPrivateMessage("peer2", "bob", "peer1", "alice", "Hi from Bob");
  sendPrivateMessage("peer3", "charlie", "peer1", "alice", "Hi from Charlie");

  chat::InformClientsNewMessageResponse response;

  auto status1 = broadcaster_->nextPrivateMessage(
      "peer1", std::chrono::milliseconds(0), response);
  EXPECT_EQ(status1, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(response.author(), "bob");
  EXPECT_EQ(response.content(), "Hi from Bob");

  auto status2 = broadcaster_->nextPrivateMessage(
      "peer1", std::chrono::milliseconds(0), response);
  EXPECT_EQ(status2, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(response.author(), "charlie");
  EXPECT_EQ(response.content(), "Hi from Charlie");
}

// --- normalizePrivateMessageIndex Tests ---

TEST_F(PrivateMessageBroadcasterTest,
       NormalizePrivateMessageIndex_PeerNotConnected_ReturnsFalse) {
  bool result = broadcaster_->normalizePrivateMessageIndex("unknown_peer");
  EXPECT_FALSE(result);
}

TEST_F(PrivateMessageBroadcasterTest,
       NormalizePrivateMessageIndex_ConnectedPeer_ReturnsTrue) {
  connectClient("peer1", "alice");

  bool result = broadcaster_->normalizePrivateMessageIndex("peer1");
  EXPECT_TRUE(result);
}

TEST_F(PrivateMessageBroadcasterTest,
       NormalizePrivateMessageIndex_InitializesEmptyQueue) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");

  // Send message before normalize
  sendPrivateMessage("peer1", "alice", "peer2", "bob", "Message before init");

  // Now normalize - should still have the message in queue
  bool result = broadcaster_->normalizePrivateMessageIndex("peer2");
  EXPECT_TRUE(result);

  // Message sent before normalize should still be available
  chat::InformClientsNewMessageResponse response;
  auto status = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(0), response);
  EXPECT_EQ(status, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(response.content(), "Message before init");
}

// --- onPrivateMessageSent Tests ---

TEST_F(PrivateMessageBroadcasterTest, OnPrivateMessageSent_AddsMessageToQueue) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");

  broadcaster_->normalizePrivateMessageIndex("peer2");

  events::PrivateMessageSentEvent event{
      .senderPeer = "peer1",
      .senderPseudonym = "alice",
      .recipientPeer = "peer2",
      .recipientPseudonym = "bob",
      .content = "Test private content",
  };
  broadcaster_->onPrivateMessageSent(event);

  chat::InformClientsNewMessageResponse response;
  auto status = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(0), response);

  EXPECT_EQ(status, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(response.author(), "alice");
  EXPECT_EQ(response.content(), "Test private content");
  EXPECT_TRUE(response.isprivate());
}

TEST_F(PrivateMessageBroadcasterTest, OnPrivateMessageSent_WakesWaitingPeer) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");

  broadcaster_->normalizePrivateMessageIndex("peer2");

  std::atomic<NextPrivateMessageStatus> status{
      NextPrivateMessageStatus::kNoMessage};
  std::atomic<bool> messageReceived{false};

  std::thread waitingThread([this, &status, &messageReceived]() {
    chat::InformClientsNewMessageResponse response;
    status = broadcaster_->nextPrivateMessage(
        "peer2", std::chrono::milliseconds(500), response);
    if (status == NextPrivateMessageStatus::kOk) {
      messageReceived = true;
    }
  });

  // Give the thread time to start waiting
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Send a private message which should wake up the waiting thread
  sendPrivateMessage("peer1", "alice", "peer2", "bob", "Wake up!");

  waitingThread.join();

  EXPECT_EQ(status.load(), NextPrivateMessageStatus::kOk);
  EXPECT_TRUE(messageReceived.load());
}

// --- Observer interface Tests ---

TEST_F(PrivateMessageBroadcasterTest, OnClientConnected_NoOp) {
  events::ClientConnectedEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .gender = "female",
      .country = "US",
  };

  // Should not crash
  EXPECT_NO_THROW(broadcaster_->onClientConnected(event));
}

TEST_F(PrivateMessageBroadcasterTest,
       OnClientDisconnected_CleansUpDisconnectedPeerQueues) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");

  broadcaster_->normalizePrivateMessageIndex("peer1");
  broadcaster_->normalizePrivateMessageIndex("peer2");

  // Send message to bob
  sendPrivateMessage("peer1", "alice", "peer2", "bob", "Hello");

  // Disconnect bob
  disconnectClient("bob");

  // Trigger cleanup via onClientDisconnected
  events::ClientDisconnectedEvent event{
      .peer = "peer2",
      .pseudonym = "bob",
      .connectionDuration = std::chrono::seconds(60),
  };
  broadcaster_->onClientDisconnected(event);

  // Bob's queue should be cleaned up, attempting to get messages returns
  // PeerMissing
  chat::InformClientsNewMessageResponse response;
  auto status = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(0), response);
  EXPECT_EQ(status, NextPrivateMessageStatus::kPeerMissing);
}

TEST_F(PrivateMessageBroadcasterTest, OnMessageSent_NoOp) {
  events::MessageSentEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .content = "Public message",
  };

  // Should not crash and should not affect private message queues
  EXPECT_NO_THROW(broadcaster_->onMessageSent(event));
}

// --- Edge cases ---

TEST_F(PrivateMessageBroadcasterTest,
       NextPrivateMessage_QueueConsumed_ReturnsNoMessage) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");

  broadcaster_->normalizePrivateMessageIndex("peer2");

  sendPrivateMessage("peer1", "alice", "peer2", "bob", "Only message");

  chat::InformClientsNewMessageResponse response;

  // First call consumes the message
  auto status1 = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(0), response);
  EXPECT_EQ(status1, NextPrivateMessageStatus::kOk);

  // Second call should return NoMessage
  auto status2 = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(10), response);
  EXPECT_EQ(status2, NextPrivateMessageStatus::kNoMessage);
}

TEST_F(PrivateMessageBroadcasterTest,
       NextPrivateMessage_IndependentQueuesPerPeer) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");
  connectClient("peer3", "charlie");

  broadcaster_->normalizePrivateMessageIndex("peer2");
  broadcaster_->normalizePrivateMessageIndex("peer3");

  // Send different messages to different recipients
  sendPrivateMessage("peer1", "alice", "peer2", "bob", "For Bob");
  sendPrivateMessage("peer1", "alice", "peer3", "charlie", "For Charlie");

  chat::InformClientsNewMessageResponse bobResponse;
  auto bobStatus = broadcaster_->nextPrivateMessage(
      "peer2", std::chrono::milliseconds(0), bobResponse);
  EXPECT_EQ(bobStatus, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(bobResponse.content(), "For Bob");

  chat::InformClientsNewMessageResponse charlieResponse;
  auto charlieStatus = broadcaster_->nextPrivateMessage(
      "peer3", std::chrono::milliseconds(0), charlieResponse);
  EXPECT_EQ(charlieStatus, NextPrivateMessageStatus::kOk);
  EXPECT_EQ(charlieResponse.content(), "For Charlie");
}

} // namespace
} // namespace domain
