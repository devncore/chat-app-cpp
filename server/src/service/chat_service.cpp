#include "service/chat_service.hpp"

#include <format>
#include <iostream>

#include "service/validation/validators/content_validator.hpp"
#include "service/validation/validators/rate_limit_validator.hpp"

using namespace std::chrono_literals;

ChatService::ChatService(
    std::shared_ptr<domain::ClientRegistry> clientRegistry,
    std::shared_ptr<domain::IMessageBroadcaster> messageBroadcaster,
    std::shared_ptr<domain::IClientEventBroadcaster> clientEventBroadcaster,
    events::EventDispatcher *eventDispatcher)
    : clientRegistry_(std::move(clientRegistry)),
      messageBroadcaster_(std::move(messageBroadcaster)),
      clientEventBroadcaster_(std::move(clientEventBroadcaster)),
      eventDispatcher_(eventDispatcher) {
  validationChain_
      .add(std::make_shared<service::validation::ContentValidator>())
      .add(std::make_shared<service::validation::RateLimitValidator>(1s));
}

ChatService::~ChatService() = default;

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

  if (!clientRegistry_->isPseudonymAvailable(peerAddress,
                                             request->pseudonym())) {
    response->set_accepted(false);
    response->set_message("The pseudo you are using is already in use, please "
                          "choose another one");
    return grpc::Status::OK;
  }

  response->set_accepted(true);
  response->set_message("New client '" + request->pseudonym() +
                        "' is now connected");
  std::cout << response->message() << std::endl;

  // Populate the initial roster of connected pseudonyms
  const auto connectedPseudonyms = clientRegistry_->getConnectedPseudonyms();
  for (const auto &name : connectedPseudonyms) {
    response->add_connected_pseudonyms(name);
  }

  events::ClientConnectedEvent event{.peer = peerAddress,
                                     .pseudonym = request->pseudonym(),
                                     .gender = request->gender(),
                                     .country = request->country()};
  eventDispatcher_->notifyClientConnected(event);

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

  const auto connectionDuration =
      clientRegistry_->getConnectionDuration(peerAddress);

  std::cout << "'" + pseudonym + "' is disconnected" << std::endl;

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

  std::string pseudonym;
  if (!clientRegistry_->getPseudonymForPeer(peer, &pseudonym)) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                        "client not connected");
  }

  // validate the message with validators
  service::validation::ValidationContext validationCtx{
      .peer = peer,
      .pseudonym = pseudonym,
      .content = request->content(),
      .timestamp = std::chrono::steady_clock::now()};

  const auto validationResult = validationChain_.validate(validationCtx);
  if (!validationResult.valid) {
    std::cerr << std::format("[{}] Message validation failed: {}", pseudonym,
                             validationResult.errorMessage)
              << std::endl;
    return grpc::Status(validationResult.statusCode,
                        validationResult.errorMessage);
  }

  if (response != nullptr) {
    response->Clear();
  }

  std::cout << std::format("[{}] {}", pseudonym, request->content())
            << std::endl;

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

  if (!messageBroadcaster_->normalizeMessageIndex(peer)) {
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
        messageBroadcaster_->nextMessage(peer, 200ms, &nextMessage);

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

  if (!clientRegistry_->isPeerConnected(peer)) {
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                        "client not connected");
  }

  clientEventBroadcaster_->normalizeClientEventIndex(peer);

  using namespace std::chrono_literals;
  while (true) {
    if (context != nullptr && context->IsCancelled()) {
      return grpc::Status::CANCELLED;
    }

    chat::ClientEventData nextEvent;
    const domain::NextClientEventStatus status =
        clientEventBroadcaster_->nextClientEvent(peer, 200ms, &nextEvent);

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
