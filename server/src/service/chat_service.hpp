#pragma once

#include <memory>

#include <google/protobuf/empty.pb.h>
#include <grpcpp/grpcpp.h>

#include "chat.grpc.pb.h"
#include "domain/chat_room.hpp"
#include "repository/database_repository.hpp"

class ChatService final : public chat::ChatService::Service {
public:
  explicit ChatService(std::weak_ptr<database::IDatabaseRepository> db);

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
  domain::ChatRoom chatRoom_;
  std::weak_ptr<database::IDatabaseRepository> db_;

  std::shared_ptr<database::IDatabaseRepository>
  getSharedDatabaseRepository() const;
};
