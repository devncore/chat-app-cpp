#include <gtest/gtest.h>

#include "mock/mock_service_event_observer.hpp"
#include "service/events/chat_service_events_dispatcher.hpp"

#include <memory>

namespace events {
namespace {

class EventDispatcherTest : public ::testing::Test {
protected:
  EventDispatcher dispatcher_;

  events::ClientConnectedEvent makeConnectedEvent(
      const std::string &peer = "peer1",
      const std::string &pseudonym = "alice",
      const std::string &gender = "female",
      const std::string &country = "US") {
    return {.peer = peer,
            .pseudonym = pseudonym,
            .gender = gender,
            .country = country};
  }

  events::ClientDisconnectedEvent makeDisconnectedEvent(
      const std::string &peer = "peer1",
      const std::string &pseudonym = "alice",
      std::chrono::steady_clock::duration duration = std::chrono::seconds(60)) {
    return {
        .peer = peer, .pseudonym = pseudonym, .connectionDuration = duration};
  }

  events::MessageSentEvent
  makeMessageEvent(const std::string &peer = "peer1",
                   const std::string &pseudonym = "alice",
                   const std::string &content = "Hello, World!") {
    return {.peer = peer, .pseudonym = pseudonym, .content = content};
  }
};

TEST_F(EventDispatcherTest, InitiallyHasNoObservers) {
  // Notifying with no observers should not crash
  EXPECT_NO_THROW(dispatcher_.notifyClientConnected(makeConnectedEvent()));
  EXPECT_NO_THROW(dispatcher_.notifyClientDisconnected(makeDisconnectedEvent()));
  EXPECT_NO_THROW(dispatcher_.notifyMessageSent(makeMessageEvent()));
}

TEST_F(EventDispatcherTest, RegisterObserver_NotifiesOnClientConnected) {
  auto observer = std::make_shared<mock::MockServiceEventObserver>();
  dispatcher_.registerObserver(observer);

  auto event = makeConnectedEvent("peer1", "alice", "female", "FR");
  dispatcher_.notifyClientConnected(event);

  ASSERT_EQ(observer->clientConnectedEvents.size(), 1);
  EXPECT_EQ(observer->clientConnectedEvents[0].peer, "peer1");
  EXPECT_EQ(observer->clientConnectedEvents[0].pseudonym, "alice");
  EXPECT_EQ(observer->clientConnectedEvents[0].gender, "female");
  EXPECT_EQ(observer->clientConnectedEvents[0].country, "FR");
}

TEST_F(EventDispatcherTest, RegisterObserver_NotifiesOnClientDisconnected) {
  auto observer = std::make_shared<mock::MockServiceEventObserver>();
  dispatcher_.registerObserver(observer);

  auto event = makeDisconnectedEvent("peer2", "bob", std::chrono::seconds(120));
  dispatcher_.notifyClientDisconnected(event);

  ASSERT_EQ(observer->clientDisconnectedEvents.size(), 1);
  EXPECT_EQ(observer->clientDisconnectedEvents[0].peer, "peer2");
  EXPECT_EQ(observer->clientDisconnectedEvents[0].pseudonym, "bob");
  EXPECT_EQ(observer->clientDisconnectedEvents[0].connectionDuration,
            std::chrono::seconds(120));
}

TEST_F(EventDispatcherTest, RegisterObserver_NotifiesOnMessageSent) {
  auto observer = std::make_shared<mock::MockServiceEventObserver>();
  dispatcher_.registerObserver(observer);

  auto event = makeMessageEvent("peer1", "alice", "Test message");
  dispatcher_.notifyMessageSent(event);

  ASSERT_EQ(observer->messageSentEvents.size(), 1);
  EXPECT_EQ(observer->messageSentEvents[0].peer, "peer1");
  EXPECT_EQ(observer->messageSentEvents[0].pseudonym, "alice");
  EXPECT_EQ(observer->messageSentEvents[0].content, "Test message");
}

TEST_F(EventDispatcherTest, MultipleObservers_AllReceiveEvents) {
  auto observer1 = std::make_shared<mock::MockServiceEventObserver>();
  auto observer2 = std::make_shared<mock::MockServiceEventObserver>();
  auto observer3 = std::make_shared<mock::MockServiceEventObserver>();

  dispatcher_.registerObserver(observer1);
  dispatcher_.registerObserver(observer2);
  dispatcher_.registerObserver(observer3);

  dispatcher_.notifyClientConnected(makeConnectedEvent());

  EXPECT_EQ(observer1->clientConnectedEvents.size(), 1);
  EXPECT_EQ(observer2->clientConnectedEvents.size(), 1);
  EXPECT_EQ(observer3->clientConnectedEvents.size(), 1);
}

TEST_F(EventDispatcherTest, ExpiredObserver_IsSkipped) {
  auto strongObserver = std::make_shared<mock::MockServiceEventObserver>();
  dispatcher_.registerObserver(strongObserver);

  {
    auto tempObserver = std::make_shared<mock::MockServiceEventObserver>();
    dispatcher_.registerObserver(tempObserver);
    // tempObserver goes out of scope here
  }

  // Should not crash when notifying with an expired weak_ptr
  EXPECT_NO_THROW(dispatcher_.notifyClientConnected(makeConnectedEvent()));

  // The strong observer should still receive the event
  EXPECT_EQ(strongObserver->clientConnectedEvents.size(), 1);
}

TEST_F(EventDispatcherTest, MultipleEvents_AllDelivered) {
  auto observer = std::make_shared<mock::MockServiceEventObserver>();
  dispatcher_.registerObserver(observer);

  dispatcher_.notifyClientConnected(makeConnectedEvent("p1", "alice"));
  dispatcher_.notifyClientConnected(makeConnectedEvent("p2", "bob"));
  dispatcher_.notifyMessageSent(makeMessageEvent("p1", "alice", "msg1"));
  dispatcher_.notifyMessageSent(makeMessageEvent("p2", "bob", "msg2"));
  dispatcher_.notifyClientDisconnected(makeDisconnectedEvent("p1", "alice"));

  EXPECT_EQ(observer->clientConnectedEvents.size(), 2);
  EXPECT_EQ(observer->messageSentEvents.size(), 2);
  EXPECT_EQ(observer->clientDisconnectedEvents.size(), 1);
}

TEST_F(EventDispatcherTest, EventsPreserveOrder) {
  auto observer = std::make_shared<mock::MockServiceEventObserver>();
  dispatcher_.registerObserver(observer);

  dispatcher_.notifyClientConnected(makeConnectedEvent("p1", "first"));
  dispatcher_.notifyClientConnected(makeConnectedEvent("p2", "second"));
  dispatcher_.notifyClientConnected(makeConnectedEvent("p3", "third"));

  ASSERT_EQ(observer->clientConnectedEvents.size(), 3);
  EXPECT_EQ(observer->clientConnectedEvents[0].pseudonym, "first");
  EXPECT_EQ(observer->clientConnectedEvents[1].pseudonym, "second");
  EXPECT_EQ(observer->clientConnectedEvents[2].pseudonym, "third");
}

TEST_F(EventDispatcherTest, RegisterSameObserverTwice_ReceivesEventsTwice) {
  auto observer = std::make_shared<mock::MockServiceEventObserver>();
  dispatcher_.registerObserver(observer);
  dispatcher_.registerObserver(observer);

  dispatcher_.notifyClientConnected(makeConnectedEvent());

  // Observer is registered twice, so it receives the event twice
  EXPECT_EQ(observer->clientConnectedEvents.size(), 2);
}

} // namespace
} // namespace events
