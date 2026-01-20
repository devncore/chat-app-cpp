#pragma once

#include <chrono>
#include <mutex>
#include <unordered_map>

#include "service/validation/message_validator.hpp"

namespace service::validation {

class RateLimitValidator : public IMessageValidator {
public:
  explicit RateLimitValidator(std::chrono::milliseconds minInterval)
      : minInterval_(minInterval) {}

  ValidationResult validate(const ValidationContext &ctx) override {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = lastMessageTime_.find(ctx.peer);
    if (it != lastMessageTime_.end()) {
      const auto elapsed = ctx.timestamp - it->second;
      if (elapsed < minInterval_) {
        return ValidationResult::failure("You are sending messages too fast",
                                         grpc::StatusCode::RESOURCE_EXHAUSTED);
      }
    }

    lastMessageTime_[ctx.peer] = ctx.timestamp;
    return ValidationResult::success();
  }

private:
  std::chrono::milliseconds minInterval_;
  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::chrono::steady_clock::time_point>
      lastMessageTime_;
};

} // namespace service::validation
