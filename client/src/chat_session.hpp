#pragma once

#include <functional>
#include <string>

#include <grpcpp/grpcpp.h>

#include "chat.grpc.pb.h"

class IChatSession {
public:
  using MessageCallback =
      std::function<void(const chat::InformClientsNewMessageResponse &)>;
  using ClientEventCallback =
      std::function<void(const chat::ClientEventData &)>;
  using ErrorCallback = std::function<void(const std::string &)>;

  struct ConnectResult {
    grpc::Status status;
    chat::ConnectResponse response;
  };

  virtual ~IChatSession() = default;

  virtual ConnectResult connect(const std::string &pseudonym,
                                const std::string &gender,
                                const std::string &country) = 0;
  virtual grpc::Status disconnect(const std::string &pseudonym) = 0;
  virtual grpc::Status sendMessage(const std::string &content) = 0;

  virtual void startMessageStream(MessageCallback onMessage,
                                  ErrorCallback onError) = 0;
  virtual void stopMessageStream() = 0;
  virtual void startClientEventStream(ClientEventCallback onEvent,
                                      ErrorCallback onError) = 0;
  virtual void stopClientEventStream() = 0;
};
