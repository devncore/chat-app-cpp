#include "grpc/grpc_runner.hpp"

#include <iostream>
#include <stdexcept>

GrpcRunner::GrpcRunner(std::shared_ptr<database::IDatabaseManager> db,
                       std::string_view serverAddress)
    : clientRegistry_(std::make_shared<domain::ClientRegistry>()),
      messageBroadcaster_(
          std::make_shared<domain::MessageBroadcaster>(*clientRegistry_)),
      clientEventBroadcaster_(
          std::make_shared<domain::ClientEventBroadcaster>(*clientRegistry_)),
      privateMessageBroadcaster_(
          std::make_shared<domain::PrivateMessageBroadcaster>(*clientRegistry_)),
      dbLogger_(std::make_shared<observers::DatabaseEventLogger>(db)) {
  // Register observers with the event dispatcher
  // ClientRegistry must be registered first to update state before other
  // observers
  eventDispatcher_.registerObserver(clientRegistry_);
  eventDispatcher_.registerObserver(
      std::static_pointer_cast<events::IServiceEventObserver>(
          messageBroadcaster_));
  eventDispatcher_.registerObserver(dbLogger_);
  eventDispatcher_.registerObserver(
      std::static_pointer_cast<events::IServiceEventObserver>(
          clientEventBroadcaster_));
  eventDispatcher_.registerObserver(
      std::static_pointer_cast<events::IServiceEventObserver>(
          privateMessageBroadcaster_));

  // Create ChatService with dependencies
  service_ = std::make_unique<ChatService>(
      clientRegistry_, messageBroadcaster_, privateMessageBroadcaster_,
      clientEventBroadcaster_, &eventDispatcher_);

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

  serverThread_ = std::jthread([this](const std::stop_token &) {
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

void GrpcRunner::wait() {
  if (serverThread_.joinable()) {
    serverThread_.join();
  }
}
