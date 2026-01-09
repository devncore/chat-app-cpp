#include "service/chat_service_impl.hpp"

#include <chrono>
#include <format>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

ChatServiceImpl::ChatServiceImpl(
    std::shared_ptr<database::IDatabaseRepository> db)
    : db_(std::move(db)) {}

grpc::Status ChatServiceImpl::Connect(grpc::ServerContext *context,
                                      const chat::ConnectRequest *request,
                                      chat::ConnectResponse *response) {
  if (request == nullptr || request->pseudonym().empty()) {
    response->set_accepted(false);
    response->set_message("pseudonym is required");
    return grpc::Status::OK;
  }

  const std::string peerAddress = context ? context->peer() : std::string{};
  if (peerAddress.empty()) {
    response->set_accepted(false);
    response->set_message("peer information is required");
    return grpc::Status::OK;
  }

  const domain::ConnectResult result =
      chat_room_.connectClient(peerAddress, request->pseudonym(),
                               request->gender(), request->country());

  response->set_accepted(result.accepted);
  response->set_message(result.message);
  std::cout << result.message << std::endl;

  if (result.accepted) {
    db_->clientConnectionEvent(request->pseudonym());
  }

  return grpc::Status::OK;
}

grpc::Status ChatServiceImpl::Disconnect(grpc::ServerContext *context,
                                         const chat::DisconnectRequest *request,
                                         google::protobuf::Empty *response) {
  (void)response;

  const auto &pseudonym = request->pseudonym();

  if (request == nullptr || pseudonym.empty()) {
    return grpc::Status::OK;
  }

  const std::string peerAddress = context ? context->peer() : std::string{};
  if (peerAddress.empty()) {
    return grpc::Status::OK;
  }

  chat_room_.disconnectClient(pseudonym);

  std::cout << "'" + pseudonym + "' is disconnected" << std::endl;

  return grpc::Status::OK;
}

grpc::Status ChatServiceImpl::SendMessage(
    grpc::ServerContext *context, const chat::SendMessageRequest *request,
    google::protobuf::Empty *response) {
  if (request == nullptr || request->content().empty()) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "message content is required");
  }

  const std::string peer = context ? context->peer() : std::string{};
  if (peer.empty()) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                        "peer information missing");
  }

  std::string pseudonym;
  if (!chat_room_.getPseudonymForPeer(peer, &pseudonym)) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                        "client not connected");
  }

  if (response) {
    response->Clear();
  }

  std::cout << std::format("[{}] {}", pseudonym, request->content())
            << std::endl;

  chat_room_.addMessage(pseudonym, request->content());

  db_->incrementTxMessage(pseudonym);

  return grpc::Status::OK;
}

grpc::Status ChatServiceImpl::SubscribeMessages(
    grpc::ServerContext *context,
    const chat::InformClientsNewMessageRequest *request,
    grpc::ServerWriter<chat::InformClientsNewMessageResponse> *writer) {
  if (request == nullptr) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "request is required");
  }

  const std::string peer = context ? context->peer() : std::string{};
  if (peer.empty()) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                        "peer information missing");
  }

  if (!chat_room_.normalizeMessageIndex(peer)) {
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
        chat_room_.nextMessage(peer, 200ms, &nextMessage);

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

grpc::Status ChatServiceImpl::SubscribeClientEvents(
    grpc::ServerContext *context, const google::protobuf::Empty *request,
    grpc::ServerWriter<chat::ClientEventData> *writer) {
  (void)request;
  if (writer == nullptr) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "writer is required");
  }

  const std::string peer = context ? context->peer() : std::string{};
  if (peer.empty()) {
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                        "peer information missing");
  }

  std::vector<std::string> connectedPseudonyms;
  if (!chat_room_.getInitialRoster(peer, &connectedPseudonyms)) {
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
        chat_room_.nextClientEvent(peer, 200ms, &nextEvent);

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
