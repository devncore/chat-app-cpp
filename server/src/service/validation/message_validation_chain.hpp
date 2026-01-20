#pragma once

#include <memory>
#include <vector>

#include "service/validation/message_validator.hpp"

namespace service::validation {

class MessageValidationChain {
public:
  MessageValidationChain &add(std::shared_ptr<IMessageValidator> validator) {
    validators_.push_back(std::move(validator));
    return *this;
  }

  ValidationResult validate(const ValidationContext &ctx) const {
    for (const auto &validator : validators_) {
      auto result = validator->validate(ctx);
      if (!result.valid) {
        return result;
      }
    }
    return ValidationResult::success();
  }

  bool empty() const { return validators_.empty(); }

private:
  std::vector<std::shared_ptr<IMessageValidator>> validators_;
};

} // namespace service::validation
