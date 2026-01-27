#pragma once

#include <expected>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace database {

using OptionalErrorMessage = std::optional<std::string>;

/**
 * @brief Interface for persistence operations used by the client.
 */
class IDatabaseManager {
public:
  /**
   * @brief Virtual destructor for interface cleanup.
   */
  virtual ~IDatabaseManager() = default;

  /**
   * @brief Filter the given pseudonyms against the banned_users table.
   * @param pseudonyms The list of pseudonyms to check.
   * @return The subset of pseudonyms that are banned, or error message on
   * failure.
   */
  [[nodiscard]] virtual std::expected<std::vector<std::string>, std::string>
  isBannedUsers(std::span<const std::string> pseudonyms) noexcept = 0;
};

} // namespace database
