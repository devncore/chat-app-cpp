#pragma once

#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <string>
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

  [[nodiscard]] OptionalErrorMessage init();
  [[nodiscard]] std::expected<std::vector<std::string>, std::string>
  isBannedUsers(std::span<const std::string> pseudonyms) noexcept override;

private:
  [[nodiscard]] OptionalErrorMessage ensureOpen();

  const std::string bannedUsersTable_ = "banned_users";
  std::string dbPath_;
  std::unique_ptr<SQLite::Database> db_;
};

} // namespace database
