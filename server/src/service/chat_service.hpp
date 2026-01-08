#pragma once

#include <memory>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/empty.pb.h>

#include "chat.grpc.pb.h"
#include "domain/chat_room.hpp"

namespace database {
class DatabaseManager;
}

class ChatServiceImpl final : public chat::ChatService::Service {
public:
  explicit ChatServiceImpl(std::shared_ptr<database::DatabaseManager> db);

  grpc::Status Connect(grpc::ServerContext *context,
                       const chat::ConnectRequest *request,
                       chat::ConnectResponse *response) override;

  grpc::Status Disconnect(grpc::ServerContext *context,
                          const chat::DisconnectRequest *request,
                          google::protobuf::Empty *response) override;

  grpc::Status
  InformServerNewMessage(grpc::ServerContext *context,
                         const chat::SendMessageRequest *request,
                         google::protobuf::Empty *response) override;

  grpc::Status InformClientsNewMessage(
      grpc::ServerContext *context,
      const chat::InformClientsNewMessageRequest *request,
      grpc::ServerWriter<chat::InformClientsNewMessageResponse> *writer)
      override;

  grpc::Status InformClientsClientEvent(
      grpc::ServerContext *context, const google::protobuf::Empty *request,
      grpc::ServerWriter<chat::ClientEventData> *writer) override;

private:
  domain::ChatRoom chat_room_;
  std::shared_ptr<database::DatabaseManager> db_;
};
