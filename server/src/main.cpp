#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "chat.grpc.pb.h"
#include <format>
#include <google/protobuf/empty.pb.h>

struct ClientInfo {
  std::string pseudonym;
  std::string gender;
  std::string country;
  std::size_t next_message_index{0};
};

class ChatServiceImpl final : public chat::ChatService::Service {
public:
  grpc::Status Connect(grpc::ServerContext *context,
                       const chat::ConnectRequest *request,
                       chat::ConnectResponse *response) override {
    if (request == nullptr || request->pseudonym().empty()) {
      response->set_accepted(false);
      response->set_message("pseudonym is required");
      return grpc::Status::OK;
    }

    const std::string peer_address = context ? context->peer() : std::string{};
    if (peer_address.empty()) {
      response->set_accepted(false);
      response->set_message("peer information is required");
      return grpc::Status::OK;
    }

    std::cout << "peer address = " << peer_address << std::endl;
    // std::cout << "peer Id = " <<  << std::endl;

    std::string log_message;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      const bool pseudonymInUse =
          std::any_of(clients_.cbegin(), clients_.cend(),
                      [&request, &peer_address](const auto &entry) {
                        return entry.first != peer_address &&
                               entry.second.pseudonym == request->pseudonym();
                      });
      if (pseudonymInUse) {
        log_message = "The pseudo you are using is already in use, please "
                      "choose another one";
        response->set_accepted(false);
        response->set_message(log_message);
        std::cout << log_message << std::endl;
        return grpc::Status::OK;
      }

      ClientInfo info{
          .pseudonym = request->pseudonym(),
          .gender = request->gender(),
          .country = request->country(),
          .next_message_index = message_history_.size(),
      };

      log_message =
          "New client '" + request->pseudonym() + "' is now connected";
      auto it = clients_.find(peer_address);
      if (it != clients_.end()) {
        it->second = std::move(info);
      } else {
        clients_.emplace(peer_address, std::move(info));
      }
    }

    response->set_accepted(true);
    response->set_message(log_message);
    std::cout << log_message << std::endl;

    return grpc::Status::OK;
  }

  grpc::Status
  InformServerNewMessage(grpc::ServerContext *context,
                         const chat::SendMessageRequest *request,
                         google::protobuf::Empty *response) override {
    if (request == nullptr || request->content().empty()) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "message content is required");
    }

    // check context
    const std::string peer = context ? context->peer() : std::string{};
    if (peer.empty()) {
      return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                          "peer information missing");
    }

    // check if client is connected
    std::string pseudonym;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      const auto it = clients_.find(peer);
      if (it == clients_.end()) {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                            "client not connected");
      }
      pseudonym = it->second.pseudonym;
    }

    if (response) {
      response->Clear();
    }

    std::cout << std::format("[{}] {}", pseudonym, request->content())
              << std::endl;

    broadcastMessage(pseudonym, request->content());

    return grpc::Status::OK;
  }

  grpc::Status InformClientsNewMessage(
      grpc::ServerContext *context,
      const chat::InformClientsNewMessageRequest *request,
      grpc::ServerWriter<chat::InformClientsNewMessageResponse> *writer)
      override {
    if (request == nullptr) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "request is required");
    }

    const std::string peer = context ? context->peer() : std::string{};
    if (peer.empty()) {
      return grpc::Status(grpc::StatusCode::UNAUTHENTICATED,
                          "peer information missing");
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = clients_.find(peer);
      if (it == clients_.end()) {
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                            "client not connected");
      }

      if (it->second.next_message_index > message_history_.size()) {
        it->second.next_message_index = message_history_.size();
      }
    }

    using namespace std::chrono_literals;
    while (true) {
      chat::InformClientsNewMessageResponse nextMessage;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        while (true) {
          if (context != nullptr && context->IsCancelled()) {
            return grpc::Status::CANCELLED;
          }

          auto it = clients_.find(peer);
          if (it == clients_.end()) {
            return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                                "client not connected");
          }

          if (it->second.next_message_index < message_history_.size()) {
            nextMessage = message_history_[it->second.next_message_index];
            ++it->second.next_message_index;
            break;
          }

          message_cv_.wait_for(lock, 200ms);
        }
      }

      if (!writer->Write(nextMessage)) {
        return grpc::Status(grpc::StatusCode::UNKNOWN,
                            "failed to write to client stream");
      }
    }
  }

private:
  void broadcastMessage(const std::string &author, const std::string &content) {
    chat::InformClientsNewMessageResponse payload;
    payload.set_author(author);
    payload.set_content(content);

    {
      std::lock_guard<std::mutex> lock(mutex_);
      message_history_.push_back(payload);
    }

    message_cv_.notify_all();
  }

  std::mutex mutex_;
  std::condition_variable message_cv_;
  std::vector<chat::InformClientsNewMessageResponse> message_history_;
  std::unordered_map<std::string, ClientInfo> clients_;
};

int main() {
  const std::string server_address{"0.0.0.0:50051"};
  ChatServiceImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  if (!server) {
    std::cerr << "Failed to start gRPC server." << std::endl;
    return 1;
  }

  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();

  return 0;
}
