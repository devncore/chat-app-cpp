#pragma once

#include "service/validation/validation_types.hpp"

namespace service::validation {

class IMessageValidator {
public:
  virtual ~IMessageValidator() = default;

  virtual ValidationResult validate(const ValidationContext &ctx) = 0;
};

} // namespace service::validation
