#pragma once

#include <memory>
#include <string_view>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "database/database_event_logger.hpp"
#include "database/database_manager.hpp"
#include "domain/client_event_broadcaster.hpp"
#include "domain/client_registry.hpp"
#include "domain/message_broadcaster.hpp"
#include "service/chat_service.hpp"
#include "service/events/chat_service_events_dispatcher.hpp"

class GrpcRunner {
public:
  GrpcRunner(std::shared_ptr<database::IDatabaseManager> db,
             std::string_view serverAddress);
  ~GrpcRunner();

  void wait();

  GrpcRunner(const GrpcRunner &) = delete;
  GrpcRunner &operator=(const GrpcRunner &) = delete;
  GrpcRunner(GrpcRunner &&) = delete;
  GrpcRunner &operator=(GrpcRunner &&) = delete;

private:
  // Client registry (single source of truth)
  std::shared_ptr<domain::ClientRegistry> clientRegistry_;

  // Domain components
  std::shared_ptr<domain::MessageBroadcaster> messageBroadcaster_;
  std::shared_ptr<domain::ClientEventBroadcaster> clientEventBroadcaster_;

  // Observers
  std::shared_ptr<observers::DatabaseEventLogger> dbLogger_;

  // Event dispatcher
  events::EventDispatcher eventDispatcher_;

  // gRPC components
  std::unique_ptr<ChatService> service_;
  std::unique_ptr<grpc::Server> server_;
  std::jthread serverThread_;
};
