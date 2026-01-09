#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "chat.grpc.pb.h"
#include <google/protobuf/empty.pb.h>

class GrpcChatClient {
public:
  using MessageCallback =
      std::function<void(const chat::InformClientsNewMessageResponse &)>;
  using ClientEventCallback =
      std::function<void(const chat::ClientEventData &)>;
  using ErrorCallback = std::function<void(const std::string &)>;

  explicit GrpcChatClient(std::string serverAddress);
  ~GrpcChatClient();

  GrpcChatClient(const GrpcChatClient &) = delete;
  GrpcChatClient &operator=(const GrpcChatClient &) = delete;

  struct ConnectResult {
    grpc::Status status;
    chat::ConnectResponse response;
  };

  ConnectResult connect(const std::string &pseudonym, const std::string &gender,
                        const std::string &country);

  grpc::Status disconnect(const std::string &pseudonym);

  grpc::Status sendMessage(const std::string &content);

  void startMessageStream(MessageCallback onMessage, ErrorCallback onError);
  void stopMessageStream();
  void startClientEventStream(ClientEventCallback onEvent,
                              ErrorCallback onError);
  void stopClientEventStream();

private:
  void ensureStub();

  std::string serverAddress_;
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<chat::ChatService::Stub> stub_;

  std::atomic<bool> messageStreamRunning_{false};
  std::thread messageStreamThread_;
  std::shared_ptr<grpc::ClientContext> messageStreamContext_;
  std::atomic<bool> clientEventStreamRunning_{false};
  std::thread clientEventStreamThread_;
  std::shared_ptr<grpc::ClientContext> clientEventStreamContext_;
  std::mutex stubMutex_;
};
