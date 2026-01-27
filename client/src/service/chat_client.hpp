#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include <grpcpp/grpcpp.h>

#include "chat.grpc.pb.h"

/**
 * @brief Interface for a client chat session.
 */
class ChatClient {
public:
  using MessageCallback =
      std::function<void(const chat::InformClientsNewMessageResponse &)>;
  using ClientEventCallback =
      std::function<void(const chat::ClientEventData &)>;
  using ErrorCallback = std::function<void(const std::string &)>;

  /**
   * @brief Result of a connect attempt.
   */
  struct ConnectResult {
    grpc::Status status;
    chat::ConnectResponse response;
  };

  /**
   * @brief Virtual destructor for interface cleanup.
   */
  virtual ~ChatClient() = default;

  /**
   * @brief Connect the client to the server with profile data.
   * @param pseudonym The user's display name.
   * @param gender The user's gender.
   * @param country The user's country.
   * @return Status and response from the server.
   */
  virtual ConnectResult connect(std::string_view pseudonym,
                                std::string_view gender,
                                std::string_view country) = 0;

  /**
   * @brief Disconnect the client from the server.
   * @param pseudonym The user's display name.
   * @return Status returned by the server.
   */
  virtual grpc::Status disconnect(std::string_view pseudonym) = 0;

  /**
   * @brief Send a chat message to the server.
   * @param content The message content.
   * @param privateRecipient Optional pseudonym for private message recipient.
   * @return Status returned by the server.
   */
  virtual grpc::Status sendMessage(std::string_view content,
                                   const std::optional<std::string> &privateRecipient = std::nullopt) = 0;

  /**
   * @brief Start streaming new messages to the client.
   * @param onMessage Callback invoked for each incoming message.
   * @param onError Callback invoked when the stream ends with an error.
   */
  virtual void startMessageStream(MessageCallback onMessage,
                                  ErrorCallback onError) = 0;

  /**
   * @brief Stop streaming new messages to the client.
   */
  virtual void stopMessageStream() = 0;

  /**
   * @brief Start streaming client roster events.
   * @param onEvent Callback invoked for each roster event.
   * @param onError Callback invoked when the stream ends with an error.
   */
  virtual void startClientEventStream(ClientEventCallback onEvent,
                                      ErrorCallback onError) = 0;

  /**
   * @brief Stop streaming client roster events.
   */
  virtual void stopClientEventStream() = 0;
};
