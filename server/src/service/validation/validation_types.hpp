#pragma once

#include <chrono>
#include <string>

#include <grpcpp/grpcpp.h>

namespace service::validation {

struct ValidationContext {
  std::string peer;
  std::string pseudonym;
  std::string content;
  std::chrono::steady_clock::time_point timestamp;
};

struct ValidationResult {
  bool valid = true;
  std::string errorMessage;
  grpc::StatusCode statusCode = grpc::StatusCode::OK;

  static ValidationResult success() { return {true, "", grpc::StatusCode::OK}; }

  static ValidationResult failure(const std::string &message,
                                  grpc::StatusCode code) {
    return {false, message, code};
  }
};

} // namespace service::validation
