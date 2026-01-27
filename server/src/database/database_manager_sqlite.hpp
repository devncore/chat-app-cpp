#pragma once

#include <memory>
#include <optional>
#include <string>

#include "database/database_manager.hpp"

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
class DatabaseManagerSQLite : public IDatabaseManager {
public:
  DatabaseManagerSQLite();
  explicit DatabaseManagerSQLite(std::string dbPath);
  ~DatabaseManagerSQLite() override;

  [[nodiscard]] OptionalErrorMessage init();
  [[nodiscard]] OptionalErrorMessage
  clientConnectionEvent(std::string_view pseudonymStd) noexcept override;
  [[nodiscard]] OptionalErrorMessage
  incrementTxMessage(std::string_view pseudonymStd) noexcept override;
  [[nodiscard]] OptionalErrorMessage
  updateCumulatedConnectionTime(std::string_view pseudonymStd,
                                uint64_t durationInSec) noexcept override;
  [[nodiscard]] OptionalErrorMessage
  printStatisticsTableContent() noexcept override;

private:
  [[nodiscard]] OptionalErrorMessage ensureOpen();

  const std::string statisticsTable_ = "Statistics";
  std::string dbPath_;
  std::unique_ptr<SQLite::Database> db_;
};

} // namespace database
