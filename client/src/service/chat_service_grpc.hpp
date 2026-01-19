#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <qobject.h>
#include <string>
#include <thread>

#include <QString>
#include <QStringList>
#include <google/protobuf/empty.pb.h>

#include "chat_client.hpp"

class ChatServiceGrpc : public QObject, public ChatClient {
  Q_OBJECT

public:
  using ChatClient::ClientEventCallback;
  using ChatClient::ConnectResult;
  using ChatClient::ErrorCallback;
  using ChatClient::MessageCallback;

  explicit ChatServiceGrpc(std::string serverAddress,
                           QObject *parent = nullptr);
  ~ChatServiceGrpc() override;

  ChatServiceGrpc(const ChatServiceGrpc &) = delete;
  ChatServiceGrpc &operator=(const ChatServiceGrpc &) = delete;

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
                       const QString &message,
                       const QStringList &connectedPseudonyms);
  void disconnectFinished(bool ok, const QString &errorText);
  void sendMessageFinished(bool ok, const QString &errorText);
  void messageReceived(const QString &author, const QString &content);
  void messageStreamError(const QString &errorText);
  void clientEventReceived(int eventType, const QString &pseudonym);
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
