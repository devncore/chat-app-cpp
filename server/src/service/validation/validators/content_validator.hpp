#pragma once

#include <cstddef>

#include "service/validation/message_validator.hpp"

namespace service::validation {

class ContentValidator : public IMessageValidator {
public:
  struct Config {
    std::size_t maxLength = 300;
    std::size_t minLength = 2;
  };
  ContentValidator() = default;
  explicit ContentValidator(Config config) : config_(config) {}

  ValidationResult validate(const ValidationContext &ctx) override {
    if (ctx.content.length() < config_.minLength) {
      return ValidationResult::failure("Message is too short (< " +
                                           std::to_string(config_.minLength) +
                                           " characters)",
                                       grpc::StatusCode::INVALID_ARGUMENT);
    }

    if (ctx.content.length() > config_.maxLength) {
      return ValidationResult::failure("Message is too long (> " +
                                           std::to_string(config_.maxLength) +
                                           " characters)",
                                       grpc::StatusCode::INVALID_ARGUMENT);
    }

    return ValidationResult::success();
  }

private:
  Config config_;
};

} // namespace service::validation
