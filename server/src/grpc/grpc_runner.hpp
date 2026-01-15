#pragma once

#include <memory>
#include <string_view>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "database/database_manager.hpp"
#include "service/chat_service.hpp"

class GrpcRunner {
public:
  GrpcRunner(std::shared_ptr<database::IDatabaseManager> db,
             std::string_view serverAddress);
  ~GrpcRunner();

  GrpcRunner(const GrpcRunner &) = delete;
  GrpcRunner &operator=(const GrpcRunner &) = delete;
  GrpcRunner(GrpcRunner &&) = delete;
  GrpcRunner &operator=(GrpcRunner &&) = delete;

private:
  std::shared_ptr<database::IDatabaseManager> database_;
  std::unique_ptr<ChatService> service_;
  std::unique_ptr<grpc::Server> server_;
  std::jthread serverThread_;
};
