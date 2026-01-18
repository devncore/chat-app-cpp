#include "service/chat_service.hpp"

#include <format>
#include <iostream>
#include <string>
#include <vector>

ChatService::ChatService(std::shared_ptr<domain::ChatRoom> chatRoom,
                         events::EventDispatcher *eventDispatcher)
    : chatRoom_(std::move(chatRoom)), eventDispatcher_(eventDispatcher) {}

grpc::Status ChatService::Connect(grpc::ServerContext *context,
                                  const chat::ConnectRequest *request,
                                  chat::ConnectResponse *response) {
  if (request == nullptr || request->pseudonym().empty()) {
    response->set_accepted(false);
    response->set_message("pseudonym is required");
    return grpc::Status::OK;
  }

  const std::string peerAddress =
      (context != nullptr) ? context->peer() : std::string{};
  if (peerAddress.empty()) {
    response->set_accepted(false);
    response->set_message("peer information is required");
    return grpc::Status::OK;
  }

  const domain::ConnectResult result = chatRoom_->connectClient(
      peerAddress, request->pseudonym(), request->gender(), request->country());

  response->set_accepted(result.accepted);
  response->set_message(result.message);
  std::cout << result.message << std::endl;

  // Notify all observers (database logger, etc.) only if accepted
  if (result.accepted) {
    events::ClientConnectedEvent event{.peer = peerAddress,
                                       .pseudonym = request->pseudonym(),
                                       .gender = request->gender(),
                                       .country = request->country()};
    eventDispatcher_->notifyClientConnected(event);
  }

  return grpc::Status::OK;
}

grpc::Status ChatService::Disconnect(grpc::ServerContext *context,
                                     const chat::DisconnectRequest *request,
                                     google::protobuf::Empty *response) {
  (void)response;

  const auto &pseudonym = request->pseudonym();

  if (request == nullptr || pseudonym.empty()) {
    return grpc::Status::OK;
  }

  const std::string peerAddress =
      (context != nullptr) ? context->peer() : std::string{};
  if (peerAddress.empty()) {
    return grpc::Status::OK;
  }

  // Get connection duration before disconnecting
  const auto connectionDuration = chatRoom_->getConnectionDuration(peerAddress);

  // Disconnect the user
  chatRoom_->disconnectClient(pseudonym);

  std::cout << "'" + pseudonym + "' is disconnected" << std::endl;

  // Notify all observers of disconnection
  if (connectionDuration.has_value()) {
    events::ClientDisconnectedEvent event{.peer = peerAddress,
                                          .pseudonym = pseudonym,
                                          .connectionDuration =
                                              *connectionDuration};
    eventDispatcher_->notifyClientDisconnected(event);
  }

  return grpc::Status::OK;
}

grpc::Status ChatService::SendMessage(grpc::ServerContext *context,
                                      const chat::SendMessageRequest *request,
                                      google::protobuf::Empty *response) {
  if (request == nullptr || request->content().empty()) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "message content is required");
  }

  const std::string peer =
      (context != nullptr) ? context->peer() : std::string{};
  if (peer.empty()) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                        "peer information missing");
  }

  std::string pseudonym;
  if (!chatRoom_->getPseudonymForPeer(peer, &pseudonym)) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                        "client not connected");
  }

  if (response != nullptr) {
    response->Clear();
  }

  std::cout << std::format("[{}] {}", pseudonym, request->content())
            << std::endl;

  chatRoom_->addMessage(pseudonym, request->content());

  // Notify all observers (e.g., database logger)
  events::MessageSentEvent event{
      .peer = peer, .pseudonym = pseudonym, .content = request->content()};
  eventDispatcher_->notifyMessageSent(event);

  return grpc::Status::OK;
}

grpc::Status ChatService::SubscribeMessages(
    grpc::ServerContext *context,
    const chat::InformClientsNewMessageRequest *request,
    grpc::ServerWriter<chat::InformClientsNewMessageResponse> *writer) {
  if (request == nullptr) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "request is required");
  }

  const std::string peer =
      (context != nullptr) ? context->peer() : std::string{};
  if (peer.empty()) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                        "peer information missing");
  }

  if (!chatRoom_->normalizeMessageIndex(peer)) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                        "client not connected");
  }

  using namespace std::chrono_literals;
  while (true) {
    if (context != nullptr && context->IsCancelled()) {
      return grpc::Status::CANCELLED;
    }

    chat::InformClientsNewMessageResponse nextMessage;
    const domain::NextMessageStatus status =
        chatRoom_->nextMessage(peer, 200ms, &nextMessage);

    if (status == domain::NextMessageStatus::kPeerMissing) {
      return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                          "client not connected");
    }

    if (status != domain::NextMessageStatus::kOk) {
      continue;
    }

    if (!writer->Write(nextMessage)) {
      return grpc::Status(grpc::StatusCode::UNKNOWN,
                          "failed to write to client stream");
    }
  }
}

grpc::Status ChatService::SubscribeClientEvents(
    grpc::ServerContext *context, const google::protobuf::Empty *request,
    grpc::ServerWriter<chat::ClientEventData> *writer) {
  (void)request;
  if (writer == nullptr) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "writer is required");
  }

  const std::string peer =
      (context != nullptr) ? context->peer() : std::string{};
  if (peer.empty()) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                        "peer information missing");
  }

  std::vector<std::string> connectedPseudonyms;
  if (!chatRoom_->getInitialRoster(peer, &connectedPseudonyms)) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                        "client not connected");
  }

  if (!connectedPseudonyms.empty()) {
    chat::ClientEventData initialRoster;
    initialRoster.set_event_type(chat::ClientEventData::SYNC);
    for (const auto &name : connectedPseudonyms) {
      initialRoster.add_pseudonyms(name);
    }

    if (!writer->Write(initialRoster)) {
      return grpc::Status(grpc::StatusCode::UNKNOWN,
                          "failed to write initial roster");
    }
  }

  using namespace std::chrono_literals;
  while (true) {
    if (context != nullptr && context->IsCancelled()) {
      return grpc::Status::CANCELLED;
    }

    chat::ClientEventData nextEvent;
    const domain::NextClientEventStatus status =
        chatRoom_->nextClientEvent(peer, 200ms, &nextEvent);

    if (status == domain::NextClientEventStatus::kPeerMissing) {
      return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                          "client not connected");
    }

    if (status != domain::NextClientEventStatus::kOk) {
      continue;
    }

    if (!writer->Write(nextEvent)) {
      return grpc::Status(grpc::StatusCode::UNKNOWN,
                          "failed to write to client stream");
    }
  }
}
