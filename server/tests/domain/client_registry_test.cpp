#include <gtest/gtest.h>

#include "domain/client_registry.hpp"

namespace domain {
namespace {

class ClientRegistryTest : public ::testing::Test {
protected:
  ClientRegistry registry_;

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

TEST_F(ClientRegistryTest, InitiallyEmpty) {
  EXPECT_TRUE(registry_.getConnectedPseudonyms().empty());
  EXPECT_FALSE(registry_.isPeerConnected("peer1"));
}

TEST_F(ClientRegistryTest, IsPseudonymAvailable_EmptyRegistry) {
  EXPECT_TRUE(registry_.isPseudonymAvailable("peer1", "alice"));
}

TEST_F(ClientRegistryTest, IsPseudonymAvailable_TakenByOtherPeer) {
  connectClient("peer1", "alice");

  EXPECT_FALSE(registry_.isPseudonymAvailable("peer2", "alice"));
}

TEST_F(ClientRegistryTest, IsPseudonymAvailable_OwnPseudonym) {
  connectClient("peer1", "alice");

  // A peer can keep its own pseudonym
  EXPECT_TRUE(registry_.isPseudonymAvailable("peer1", "alice"));
}

TEST_F(ClientRegistryTest, IsPseudonymAvailable_DifferentPseudonym) {
  connectClient("peer1", "alice");

  EXPECT_TRUE(registry_.isPseudonymAvailable("peer2", "bob"));
}

TEST_F(ClientRegistryTest, GetPseudonymForPeer_Exists) {
  connectClient("peer1", "alice");

  std::string pseudonym;
  EXPECT_TRUE(registry_.getPseudonymForPeer("peer1", &pseudonym));
  EXPECT_EQ(pseudonym, "alice");
}

TEST_F(ClientRegistryTest, GetPseudonymForPeer_NotExists) {
  std::string pseudonym;
  EXPECT_FALSE(registry_.getPseudonymForPeer("peer1", &pseudonym));
}

TEST_F(ClientRegistryTest, GetPseudonymForPeer_NullOutput) {
  connectClient("peer1", "alice");

  // Should return true even with null output pointer
  EXPECT_TRUE(registry_.getPseudonymForPeer("peer1", nullptr));
}

TEST_F(ClientRegistryTest, IsPeerConnected_Connected) {
  connectClient("peer1", "alice");

  EXPECT_TRUE(registry_.isPeerConnected("peer1"));
}

TEST_F(ClientRegistryTest, IsPeerConnected_NotConnected) {
  EXPECT_FALSE(registry_.isPeerConnected("peer1"));
}

TEST_F(ClientRegistryTest, GetConnectedPseudonyms_MultipleClients) {
  connectClient("peer1", "alice");
  connectClient("peer2", "bob");
  connectClient("peer3", "charlie");

  auto pseudonyms = registry_.getConnectedPseudonyms();

  EXPECT_EQ(pseudonyms.size(), 3);
  EXPECT_TRUE(std::find(pseudonyms.begin(), pseudonyms.end(), "alice") !=
              pseudonyms.end());
  EXPECT_TRUE(std::find(pseudonyms.begin(), pseudonyms.end(), "bob") !=
              pseudonyms.end());
  EXPECT_TRUE(std::find(pseudonyms.begin(), pseudonyms.end(), "charlie") !=
              pseudonyms.end());
}

TEST_F(ClientRegistryTest, GetConnectionDuration_Exists) {
  connectClient("peer1", "alice");

  auto duration = registry_.getConnectionDuration("peer1");

  ASSERT_TRUE(duration.has_value());
  EXPECT_GE(duration->count(), 0);
}

TEST_F(ClientRegistryTest, GetConnectionDuration_NotExists) {
  auto duration = registry_.getConnectionDuration("peer1");

  EXPECT_FALSE(duration.has_value());
}

TEST_F(ClientRegistryTest, OnClientDisconnected_RemovesClient) {
  connectClient("peer1", "alice");
  ASSERT_TRUE(registry_.isPeerConnected("peer1"));

  disconnectClient("alice");

  EXPECT_FALSE(registry_.isPeerConnected("peer1"));
  EXPECT_TRUE(registry_.getConnectedPseudonyms().empty());
}

TEST_F(ClientRegistryTest, OnClientDisconnected_UnknownPseudonym) {
  connectClient("peer1", "alice");

  disconnectClient("unknown");

  // alice should still be connected
  EXPECT_TRUE(registry_.isPeerConnected("peer1"));
}

TEST_F(ClientRegistryTest, OnClientConnected_OverwritesExistingPeer) {
  connectClient("peer1", "alice");
  connectClient("peer1", "bob");

  std::string pseudonym;
  EXPECT_TRUE(registry_.getPseudonymForPeer("peer1", &pseudonym));
  EXPECT_EQ(pseudonym, "bob");

  auto pseudonyms = registry_.getConnectedPseudonyms();
  EXPECT_EQ(pseudonyms.size(), 1);
}

} // namespace
} // namespace domain
