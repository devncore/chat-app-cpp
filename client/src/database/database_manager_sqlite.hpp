#pragma once

#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "database/database_manager.hpp"

namespace SQLite {
class Database;
}

namespace database {

/**
 * @brief Manages database operations for the client.
 *
 * features:
 *  - Initializes and maintains a SQLite database connection.
 *  - If needed, a database connection retry is implemented on each method call.
 */
class DatabaseManagerSQLite : public IDatabaseManager {
public:
  DatabaseManagerSQLite();
  explicit DatabaseManagerSQLite(std::string dbPath);
  ~DatabaseManagerSQLite() override;

  [[nodiscard]] OptionalErrorMessage
  init(std::string_view userPseudonym) override;
  void resetConnection() override;
  bool isInitialized() const override;
  [[nodiscard]] OptionalErrorMessage
  banUser(std::string_view pseudonym) noexcept override;
  [[nodiscard]] OptionalErrorMessage
  unbanUser(std::string_view pseudonym) noexcept override;
  [[nodiscard]] std::expected<bool, std::string>
  isUserBanned(std::string_view pseudonym) noexcept override;
  [[nodiscard]] std::expected<std::vector<std::string>, std::string>
  getAllBannedUsers() noexcept override;

private:
  [[nodiscard]] OptionalErrorMessage openDatabase();
  [[nodiscard]] OptionalErrorMessage ensureOpen();

  const std::string bannedUsersTable_ = "banned_users";
  std::string dbPath_;
  std::unique_ptr<SQLite::Database> db_;
};

} // namespace database
