#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <qobject.h>
#include <string>
#include <thread>

#include <QString>
#include <QStringList>
#include <google/protobuf/empty.pb.h>

#include "chat_client_session.hpp"

class GrpcChatClient : public QObject, public ChatClientSession {
  Q_OBJECT

public:
  using ChatClientSession::ClientEventCallback;
  using ChatClientSession::ConnectResult;
  using ChatClientSession::ErrorCallback;
  using ChatClientSession::MessageCallback;

  explicit GrpcChatClient(std::string serverAddress,QObject *parent = nullptr);
  ~GrpcChatClient() override;
  
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

signals:
  void connectFinished(bool ok, const QString &errorText, bool accepted,
                       const QString &message);
  void disconnectFinished(bool ok, const QString &errorText);
  void sendMessageFinished(bool ok, const QString &errorText);
  void messageReceived(const QString &author, const QString &content);
  void messageStreamError(const QString &errorText);
  void clientEventReceived(int eventType, const QStringList &pseudonyms);
  void clientEventStreamError(const QString &errorText);

public slots:
  void connectToServer(const QString &pseudonym, const QString &gender,
                       const QString &country);
  void disconnectFromServer(const QString &pseudonym);
  void sendChatMessage(const QString &content);
  void startMessageStreamSlot();
  void stopMessageStreamSlot();
  void startClientEventStreamSlot();
  void stopClientEventStreamSlot();

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
