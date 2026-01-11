#pragma once

#include <memory>
#include <optional>
#include <string>

#include "repository/database_repository.hpp"

namespace SQLite {
class Database;
}

namespace database {

/**
 * @brief Manages database operations for the server.
 *
 * features:
 *  - Initializes and maintains a SQLite database connection.
 *  - If needed, a database connection retry is implemented on each method call.
 */
class DatabaseManager : public IDatabaseRepository {
public:
  DatabaseManager();
  explicit DatabaseManager(std::string dbPath);
  ~DatabaseManager() override;

  [[nodiscard]] OptionalErrorMessage init();
  [[nodiscard]] OptionalErrorMessage
  clientConnectionEvent(const std::string &pseudonymStd) noexcept override;
  [[nodiscard]] OptionalErrorMessage
  incrementTxMessage(const std::string &pseudonymStd) noexcept override;
  [[nodiscard]] OptionalErrorMessage
  printStatisticsTableContent() noexcept override;

private:
  [[nodiscard]] OptionalErrorMessage ensureOpen();

  const std::string statisticsTable_ = "Statistics";
  std::string dbPath_;
  std::unique_ptr<SQLite::Database> db_;
};

} // namespace database
