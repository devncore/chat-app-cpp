#pragma once

#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>

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
   * @brief Initialize the database connection.
   * @param userPseudonym The pseudonym of the current user.
   * @return std::nullopt on success, or error message on failure.
   */
  [[nodiscard]] virtual OptionalErrorMessage
  init(std::string_view userPseudonym) = 0;

  /**
   * @brief Check wether database manager is initialized
   * @return true if it is the case.
   */
  virtual bool isInitialized() const = 0;

  /**
   * @brief Filter the given pseudonyms against the banned_users table.
   * @param pseudonyms The list of pseudonyms to check.
   * @return The subset of pseudonyms that are banned, or error message on
   * failure.
   */
  [[nodiscard]] virtual std::expected<std::vector<std::string>, std::string>
  isBannedUsers(std::span<const std::string> pseudonyms) noexcept = 0;

  /**
   * @brief Ban a user by adding them to the banned_users table.
   * @param pseudonym The pseudonym of the user to ban.
   * @return std::nullopt on success, or error message on failure.
   */
  [[nodiscard]] virtual OptionalErrorMessage
  banUser(std::string_view pseudonym) noexcept = 0;

  /**
   * @brief Unban a user by removing them from the banned_users table.
   * @param pseudonym The pseudonym of the user to unban.
   * @return std::nullopt on success, or error message on failure.
   */
  [[nodiscard]] virtual OptionalErrorMessage
  unbanUser(std::string_view pseudonym) noexcept = 0;
};

} // namespace database
