#include "grpc_chat_client.hpp"

#include <qobject.h>
#include <utility>

GrpcChatClient::GrpcChatClient(std::string serverAddress, QObject *parent)
    : QObject(parent), serverAddress_(std::move(serverAddress)) {}

GrpcChatClient::~GrpcChatClient() {
  stopMessageStream();
  stopClientEventStream();
}

void GrpcChatClient::connectToServer(const QString &pseudonym,
                                     const QString &gender,
                                     const QString &country) {
  const auto result = connect(pseudonym.toStdString(), gender.toStdString(),
                              country.toStdString());
  const bool ok = result.status.ok();
  const QString errorText =
      ok ? QString{} : QString::fromStdString(result.status.error_message());
  const QString message = QString::fromStdString(result.response.message());

  emit connectFinished(ok, errorText, result.response.accepted(), message);
}

void GrpcChatClient::disconnectFromServer(const QString &pseudonym) {
  const auto status = disconnect(pseudonym.toStdString());
  const bool ok = status.ok();
  const QString errorText =
      ok ? QString{} : QString::fromStdString(status.error_message());

  emit disconnectFinished(ok, errorText);
}

void GrpcChatClient::sendChatMessage(const QString &content) {
  const auto status = sendMessage(content.toStdString());
  const bool ok = status.ok();
  const QString errorText =
      ok ? QString{} : QString::fromStdString(status.error_message());

  emit sendMessageFinished(ok, errorText);
}

void GrpcChatClient::startMessageStreamSlot() {
  startMessageStream(
      [this](const chat::InformClientsNewMessageResponse &incoming) {
        emit messageReceived(QString::fromStdString(incoming.author()),
                             QString::fromStdString(incoming.content()));
      },
      [this](const std::string &errorText) {
        if (errorText.empty()) {
          return;
        }
        emit messageStreamError(QString::fromStdString(errorText));
      });
}

void GrpcChatClient::stopMessageStreamSlot() { stopMessageStream(); }

void GrpcChatClient::startClientEventStreamSlot() {
  startClientEventStream(
      [this](const chat::ClientEventData &incoming) {
        QStringList names;
        names.reserve(incoming.pseudonyms_size());
        for (const auto &entry : incoming.pseudonyms()) {
          const auto name = QString::fromStdString(entry).trimmed();
          if (!name.isEmpty()) {
            names.append(name);
          }
        }
        emit clientEventReceived(static_cast<int>(incoming.event_type()),
                                 names);
      },
      [this](const std::string &errorText) {
        if (errorText.empty()) {
          return;
        }
        emit clientEventStreamError(QString::fromStdString(errorText));
      });
}

void GrpcChatClient::stopClientEventStreamSlot() { stopClientEventStream(); }

GrpcChatClient::ConnectResult
GrpcChatClient::connect(const std::string &pseudonym, const std::string &gender,
                        const std::string &country) {
  ensureStub();

  chat::ConnectRequest request;
  request.set_pseudonym(pseudonym);
  request.set_gender(gender);
  request.set_country(country);

  chat::ConnectResponse response;
  grpc::ClientContext context;
  const auto status = stub_->Connect(&context, request, &response);

  return {.status = status, .response = response};
}

grpc::Status GrpcChatClient::disconnect(const std::string &pseudonym) {
  ensureStub();

  chat::DisconnectRequest request;
  request.set_pseudonym(pseudonym);

  google::protobuf::Empty response;
  grpc::ClientContext context;
  const auto status = stub_->Disconnect(&context, request, &response);

  return status;
}

grpc::Status GrpcChatClient::sendMessage(const std::string &content) {
  ensureStub();
  chat::SendMessageRequest request;
  request.set_content(content);

  grpc::ClientContext context;
  google::protobuf::Empty response;
  return stub_->SendMessage(&context, request, &response);
}

void GrpcChatClient::startMessageStream(MessageCallback onMessage,
                                        ErrorCallback onError) {
  ensureStub();
  stopMessageStream();

  messageStreamRunning_.store(true);
  auto context = std::make_shared<grpc::ClientContext>();
  messageStreamContext_ = context;

  messageStreamThread_ = std::thread([this, context, onMessage, onError]() {
    chat::InformClientsNewMessageRequest request;
    auto reader = stub_->SubscribeMessages(context.get(), request);
    chat::InformClientsNewMessageResponse incoming;

    while (messageStreamRunning_.load() && reader->Read(&incoming)) {
      if (onMessage) {
        onMessage(incoming);
      }
    }

    const auto status = reader->Finish();
    const bool stillRunning = messageStreamRunning_.exchange(false);

    if (!status.ok() && stillRunning && onError) {
      onError(status.error_message());
    }
  });
}

void GrpcChatClient::stopMessageStream() {
  const bool wasRunning = messageStreamRunning_.exchange(false);
  if (wasRunning && messageStreamContext_) {
    messageStreamContext_->TryCancel();
  }

  if (messageStreamThread_.joinable()) {
    messageStreamThread_.join();
  }

  messageStreamContext_.reset();
}

void GrpcChatClient::startClientEventStream(ClientEventCallback onEvent,
                                            ErrorCallback onError) {
  ensureStub();
  stopClientEventStream();

  clientEventStreamRunning_.store(true);
  auto context = std::make_shared<grpc::ClientContext>();
  clientEventStreamContext_ = context;

  clientEventStreamThread_ = std::thread([this, context, onEvent, onError]() {
    google::protobuf::Empty request;
    auto reader = stub_->SubscribeClientEvents(context.get(), request);
    chat::ClientEventData incoming;

    while (clientEventStreamRunning_.load() && reader->Read(&incoming)) {
      if (onEvent) {
        onEvent(incoming);
      }
    }

    const auto status = reader->Finish();
    const bool stillRunning = clientEventStreamRunning_.exchange(false);

    if (!status.ok() && stillRunning && onError) {
      onError(status.error_message());
    }
  });
}

void GrpcChatClient::stopClientEventStream() {
  const bool wasRunning = clientEventStreamRunning_.exchange(false);
  if (wasRunning && clientEventStreamContext_) {
    clientEventStreamContext_->TryCancel();
  }

  if (clientEventStreamThread_.joinable()) {
    clientEventStreamThread_.join();
  }

  clientEventStreamContext_.reset();
}

void GrpcChatClient::ensureStub() {
  std::lock_guard<std::mutex> lock(stubMutex_);
  if (!stub_) {
    channel_ =
        grpc::CreateChannel(serverAddress_, grpc::InsecureChannelCredentials());
    stub_ = chat::ChatService::NewStub(channel_);
  }
}
