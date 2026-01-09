#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <google/protobuf/empty.pb.h>

#include "chat_client_session.hpp"

class GrpcChatClient : public ChatClientSession {
public:
  using ChatClientSession::ClientEventCallback;
  using ChatClientSession::ConnectResult;
  using ChatClientSession::ErrorCallback;
  using ChatClientSession::MessageCallback;

  explicit GrpcChatClient(std::string serverAddress);
  ~GrpcChatClient();

  GrpcChatClient(const GrpcChatClient &) = delete;
  GrpcChatClient &operator=(const GrpcChatClient &) = delete;

  ConnectResult connect(const std::string &pseudonym, const std::string &gender,
                        const std::string &country) override;

  grpc::Status disconnect(const std::string &pseudonym) override;

  grpc::Status sendMessage(const std::string &content) override;

  void startMessageStream(MessageCallback onMessage,
                          ErrorCallback onError) override;
  void stopMessageStream() override;
  void startClientEventStream(ClientEventCallback onEvent,
                              ErrorCallback onError) override;
  void stopClientEventStream() override;

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
