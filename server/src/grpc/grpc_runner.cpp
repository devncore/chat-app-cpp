#include "grpc/grpc_runner.hpp"

#include <iostream>
#include <stdexcept>
#include <utility>

GrpcRunner::GrpcRunner(std::shared_ptr<database::IDatabaseManager> db,
                       std::string_view serverAddress)
    : database_(std::move(db)),
      service_(std::make_unique<ChatService>(database_)) {
  const std::string serverAddressString(serverAddress);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(serverAddressString,
                           grpc::InsecureServerCredentials());
  builder.RegisterService(service_.get());

  server_ = builder.BuildAndStart();
  if (!server_) {
    throw std::runtime_error("Failed to start gRPC server.");
  }

  std::cout << "Server listening on " << serverAddressString << std::endl;

  serverThread_ = std::jthread([this](std::stop_token) {
    if (server_) {
      server_->Wait();
    }
  });
}

GrpcRunner::~GrpcRunner() {
  if (server_) {
    server_->Shutdown();
  }
}
