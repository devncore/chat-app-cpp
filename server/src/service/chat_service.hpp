#pragma once

#include <memory>

#include <google/protobuf/empty.pb.h>
#include <grpcpp/grpcpp.h>

#include "chat.grpc.pb.h"
#include "domain/client_event_broadcaster.hpp"
#include "domain/client_registry.hpp"
#include "domain/message_broadcaster.hpp"
#include "domain/private_message_broadcaster.hpp"
#include "service/events/chat_service_events_dispatcher.hpp"
#include "service/validation/message_validation_chain.hpp"

class ChatService final : public chat::ChatService::Service {
public:
  ChatService(std::shared_ptr<domain::ClientRegistry> clientRegistry,
              std::shared_ptr<domain::IMessageBroadcaster> messageBroadcaster,
              std::shared_ptr<domain::IPrivateMessageBroadcaster> privateMessageBroadcaster,
              std::shared_ptr<domain::IClientEventBroadcaster> clientEventBroadcaster,
              events::EventDispatcher *eventDispatcher);
  ~ChatService() override;

  grpc::Status Connect(grpc::ServerContext *context,
                       const chat::ConnectRequest *request,
                       chat::ConnectResponse *response) override;

  grpc::Status Disconnect(grpc::ServerContext *context,
                          const chat::DisconnectRequest *request,
                          google::protobuf::Empty *response) override;

  grpc::Status SendMessage(grpc::ServerContext *context,
                           const chat::SendMessageRequest *request,
                           google::protobuf::Empty *response) override;

  grpc::Status
  SubscribeMessages(grpc::ServerContext *context,
                    const chat::InformClientsNewMessageRequest *request,
                    grpc::ServerWriter<chat::InformClientsNewMessageResponse>
                        *writer) override;

  grpc::Status SubscribeClientEvents(
      grpc::ServerContext *context, const google::protobuf::Empty *request,
      grpc::ServerWriter<chat::ClientEventData> *writer) override;

private:
  std::shared_ptr<domain::ClientRegistry> clientRegistry_;
  std::shared_ptr<domain::IMessageBroadcaster> messageBroadcaster_;
  std::shared_ptr<domain::IPrivateMessageBroadcaster> privateMessageBroadcaster_;
  std::shared_ptr<domain::IClientEventBroadcaster> clientEventBroadcaster_;
  events::EventDispatcher *eventDispatcher_;
  service::validation::MessageValidationChain validationChain_;
};
