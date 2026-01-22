#include <gtest/gtest.h>

#include "domain/client_event_broadcaster.hpp"
#include "domain/client_registry.hpp"

#include <chrono>
#include <thread>

namespace domain {
namespace {

class ClientEventBroadcasterTest : public ::testing::Test {
protected:
  ClientRegistry registry_;
  std::unique_ptr<ClientEventBroadcaster> broadcaster_;

  void SetUp() override {
    broadcaster_ = std::make_unique<ClientEventBroadcaster>(registry_);
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
};

// --- nextClientEvent Tests ---

TEST_F(ClientEventBroadcasterTest,
       NextClientEvent_PeerNotConnected_ReturnsPeerMissing) {
  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent(
      "unknown_peer", std::chrono::milliseconds(0), response);
  EXPECT_EQ(status, NextClientEventStatus::kPeerMissing);
}

TEST_F(ClientEventBroadcasterTest,
       NextClientEvent_NoEvents_ReturnsNoEvent) {
  connectClient("peer1", "alice");

  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent("peer1",
                                              std::chrono::milliseconds(10), response);
  EXPECT_EQ(status, NextClientEventStatus::kNoEvent);
}

TEST_F(ClientEventBroadcasterTest, NextClientEvent_HasEvent_ReturnsOk) {
  connectClient("peer1", "alice");

  // Initialize peer's index first
  chat::ClientEventData unused;
  broadcaster_->nextClientEvent("peer1", std::chrono::milliseconds(0), unused);

  broadcaster_->broadcastClientEvent("bob", chat::ClientEventData::ADD);

  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent("peer1",
                                              std::chrono::milliseconds(0), response);

  EXPECT_EQ(status, NextClientEventStatus::kOk);
  EXPECT_EQ(response.event_type(), chat::ClientEventData::ADD);
  EXPECT_EQ(response.pseudonym(), "bob");
}

TEST_F(ClientEventBroadcasterTest,
       NextClientEvent_MultipleEvents_ReturnsInOrder) {
  connectClient("peer1", "alice");

  // Initialize peer's index first
  chat::ClientEventData unused;
  broadcaster_->nextClientEvent("peer1", std::chrono::milliseconds(0), unused);

  broadcaster_->broadcastClientEvent("bob", chat::ClientEventData::ADD);
  broadcaster_->broadcastClientEvent("charlie", chat::ClientEventData::ADD);
  broadcaster_->broadcastClientEvent("bob", chat::ClientEventData::REMOVE);

  chat::ClientEventData response;

  auto status1 = broadcaster_->nextClientEvent("peer1",
                                               std::chrono::milliseconds(0), response);
  EXPECT_EQ(status1, NextClientEventStatus::kOk);
  EXPECT_EQ(response.pseudonym(), "bob");
  EXPECT_EQ(response.event_type(), chat::ClientEventData::ADD);

  auto status2 = broadcaster_->nextClientEvent("peer1",
                                               std::chrono::milliseconds(0), response);
  EXPECT_EQ(status2, NextClientEventStatus::kOk);
  EXPECT_EQ(response.pseudonym(), "charlie");
  EXPECT_EQ(response.event_type(), chat::ClientEventData::ADD);

  auto status3 = broadcaster_->nextClientEvent("peer1",
                                               std::chrono::milliseconds(0), response);
  EXPECT_EQ(status3, NextClientEventStatus::kOk);
  EXPECT_EQ(response.pseudonym(), "bob");
  EXPECT_EQ(response.event_type(), chat::ClientEventData::REMOVE);
}

TEST_F(ClientEventBroadcasterTest,
       NextClientEvent_MultiplePeers_IndependentIndices) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");

  // Initialize both peers' indices first
  chat::ClientEventData unused;
  broadcaster_->nextClientEvent("peer1", std::chrono::milliseconds(0), unused);
  broadcaster_->nextClientEvent("peer2", std::chrono::milliseconds(0), unused);

  broadcaster_->broadcastClientEvent("charlie", chat::ClientEventData::ADD);

  chat::ClientEventData response1;
  chat::ClientEventData response2;

  // Both peers should see the event independently
  auto status1 = broadcaster_->nextClientEvent("peer1",
                                               std::chrono::milliseconds(0), response1);
  EXPECT_EQ(status1, NextClientEventStatus::kOk);
  EXPECT_EQ(response1.pseudonym(), "charlie");

  auto status2 = broadcaster_->nextClientEvent("peer2",
                                               std::chrono::milliseconds(0), response2);
  EXPECT_EQ(status2, NextClientEventStatus::kOk);
  EXPECT_EQ(response2.pseudonym(), "charlie");
}

TEST_F(ClientEventBroadcasterTest,
       NextClientEvent_PeerDisconnectedDuringWait_ReturnsPeerMissing) {
  connectClient("peer1", "alice");

  std::thread disconnectThread([this]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    disconnectClient("alice");
  });

  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent(
      "peer1", std::chrono::milliseconds(100), response);
  disconnectThread.join();

  EXPECT_EQ(status, NextClientEventStatus::kPeerMissing);
}

// --- broadcastClientEvent Tests ---

TEST_F(ClientEventBroadcasterTest,
       BroadcastClientEvent_EmptyPseudonym_NoEventAdded) {
  connectClient("peer1", "alice");

  broadcaster_->broadcastClientEvent("", chat::ClientEventData::ADD);

  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent("peer1",
                                              std::chrono::milliseconds(10), response);
  EXPECT_EQ(status, NextClientEventStatus::kNoEvent);
}

TEST_F(ClientEventBroadcasterTest,
       BroadcastClientEvent_AddEvent_CreatesAddPayload) {
  connectClient("peer1", "alice");

  // Initialize peer's index first
  chat::ClientEventData unused;
  broadcaster_->nextClientEvent("peer1", std::chrono::milliseconds(0), unused);

  broadcaster_->broadcastClientEvent("bob", chat::ClientEventData::ADD);

  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent("peer1",
                                              std::chrono::milliseconds(0), response);

  EXPECT_EQ(status, NextClientEventStatus::kOk);
  EXPECT_EQ(response.event_type(), chat::ClientEventData::ADD);
}

TEST_F(ClientEventBroadcasterTest,
       BroadcastClientEvent_RemoveEvent_CreatesRemovePayload) {
  connectClient("peer1", "alice");

  // Initialize peer's index first
  chat::ClientEventData unused;
  broadcaster_->nextClientEvent("peer1", std::chrono::milliseconds(0), unused);

  broadcaster_->broadcastClientEvent("bob", chat::ClientEventData::REMOVE);

  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent("peer1",
                                              std::chrono::milliseconds(0), response);

  EXPECT_EQ(status, NextClientEventStatus::kOk);
  EXPECT_EQ(response.event_type(), chat::ClientEventData::REMOVE);
}

TEST_F(ClientEventBroadcasterTest,
       BroadcastClientEvent_WakesWaitingPeers) {
  connectClient("peer1", "alice");

  std::atomic<NextClientEventStatus> status{NextClientEventStatus::kNoEvent};
  std::atomic<bool> eventReceived{false};

  std::thread waitingThread([this, &status, &eventReceived]() {
    chat::ClientEventData response;
    status = broadcaster_->nextClientEvent(
        "peer1", std::chrono::milliseconds(500), response);
    if (status == NextClientEventStatus::kOk) {
      eventReceived = true;
    }
  });

  // Give the thread time to start waiting
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Broadcast event which should wake up the waiting thread
  broadcaster_->broadcastClientEvent("bob", chat::ClientEventData::ADD);

  waitingThread.join();

  EXPECT_EQ(status.load(), NextClientEventStatus::kOk);
  EXPECT_TRUE(eventReceived.load());
}

// --- normalizeClientEventIndex Tests ---

TEST_F(ClientEventBroadcasterTest,
       NormalizeClientEventIndex_PeerNotConnected_ReturnsFalse) {
  bool result = broadcaster_->normalizeClientEventIndex("unknown_peer");
  EXPECT_FALSE(result);
}

TEST_F(ClientEventBroadcasterTest,
       NormalizeClientEventIndex_ConnectedPeer_ReturnsTrue) {
  connectClient("peer1", "alice");

  bool result = broadcaster_->normalizeClientEventIndex("peer1");
  EXPECT_TRUE(result);
}

TEST_F(ClientEventBroadcasterTest,
       NormalizeClientEventIndex_NewPeer_InitializesIndex) {
  connectClient("peer1", "alice");
  broadcaster_->broadcastClientEvent("bob", chat::ClientEventData::ADD);
  broadcaster_->broadcastClientEvent("charlie", chat::ClientEventData::ADD);

  // Normalize for a peer that hasn't requested events yet
  bool result = broadcaster_->normalizeClientEventIndex("peer1");
  EXPECT_TRUE(result);

  // After normalize, peer should start at current position (no old events)
  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent("peer1",
                                              std::chrono::milliseconds(10), response);
  EXPECT_EQ(status, NextClientEventStatus::kNoEvent);
}

// --- Observer interface Tests ---

TEST_F(ClientEventBroadcasterTest,
       OnClientConnected_BroadcastsAddEvent) {
  connectClient("peer1", "alice");

  // Initialize peer's index first
  chat::ClientEventData unused;
  broadcaster_->nextClientEvent("peer1", std::chrono::milliseconds(0), unused);

  events::ClientConnectedEvent event{
      .peer = "peer2",
      .pseudonym = "bob",
      .gender = "male",
      .country = "US",
  };
  broadcaster_->onClientConnected(event);

  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent("peer1",
                                              std::chrono::milliseconds(0), response);

  EXPECT_EQ(status, NextClientEventStatus::kOk);
  EXPECT_EQ(response.event_type(), chat::ClientEventData::ADD);
  EXPECT_EQ(response.pseudonym(), "bob");
}

TEST_F(ClientEventBroadcasterTest,
       OnClientDisconnected_BroadcastsRemoveEvent) {
  connectClient("peer1", "alice");

  // Initialize peer's index first
  chat::ClientEventData unused;
  broadcaster_->nextClientEvent("peer1", std::chrono::milliseconds(0), unused);

  events::ClientDisconnectedEvent event{
      .peer = "peer2",
      .pseudonym = "bob",
      .connectionDuration = std::chrono::seconds(60),
  };
  broadcaster_->onClientDisconnected(event);

  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent("peer1",
                                              std::chrono::milliseconds(0), response);

  EXPECT_EQ(status, NextClientEventStatus::kOk);
  EXPECT_EQ(response.event_type(), chat::ClientEventData::REMOVE);
  EXPECT_EQ(response.pseudonym(), "bob");
}

TEST_F(ClientEventBroadcasterTest, OnMessageSent_NoOp) {
  connectClient("peer1", "alice");

  events::MessageSentEvent event{
      .peer = "peer1",
      .pseudonym = "alice",
      .content = "Hello",
  };

  // Should not crash and should not add any events
  EXPECT_NO_THROW(broadcaster_->onMessageSent(event));

  chat::ClientEventData response;
  auto status = broadcaster_->nextClientEvent("peer1",
                                              std::chrono::milliseconds(10), response);
  EXPECT_EQ(status, NextClientEventStatus::kNoEvent);
}

} // namespace
} // namespace domain
