#include "grpc_chat_client.h"

#include <utility>

namespace {
constexpr char kDefaultServerAddress[] = "localhost:50051";
}

GrpcChatClient::GrpcChatClient(std::string serverAddress)
    : serverAddress_(std::move(serverAddress)) {
  if (serverAddress_.empty()) {
    serverAddress_ = kDefaultServerAddress;
  }
}

GrpcChatClient::~GrpcChatClient() {
  stopMessageStream();
  stopClientEventStream();
}

GrpcChatClient::ConnectResult
GrpcChatClient::connect(const std::string &pseudonym,
                        const std::string &gender,
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

grpc::Status GrpcChatClient::sendMessage(const std::string &content) {
  ensureStub();
  chat::SendMessageRequest request;
  request.set_content(content);

  grpc::ClientContext context;
  google::protobuf::Empty response;
  return stub_->InformServerNewMessage(&context, request, &response);
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
    auto reader = stub_->InformClientsNewMessage(context.get(), request);
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

  clientEventStreamThread_ =
      std::thread([this, context, onEvent, onError]() {
        google::protobuf::Empty request;
        auto reader =
            stub_->InformClientsClientEvent(context.get(), request);
        chat::ClientEventData incoming;

        while (clientEventStreamRunning_.load() && reader->Read(&incoming)) {
          if (onEvent) {
            onEvent(incoming);
          }
        }

        const auto status = reader->Finish();
        const bool stillRunning =
            clientEventStreamRunning_.exchange(false);

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
    channel_ = grpc::CreateChannel(serverAddress_,
                                   grpc::InsecureChannelCredentials());
    stub_ = chat::ChatService::NewStub(channel_);
  }
}
