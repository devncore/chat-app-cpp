#pragma once

#include <memory>
#include <optional>
#include <string>

#include "repository/database_repository.hpp"

namespace SQLite {
class Database;
}

namespace database {

class DatabaseManager : public IDatabaseRepository {
public:
  DatabaseManager();
  explicit DatabaseManager(std::string dbPath);
  ~DatabaseManager() override;

  void init();
  void clientConnectionEvent(const std::string &pseudonymStd) override;
  void incrementTxMessage(const std::string &pseudonymStd) override;
  void printStatisticsTableContent() override;

private:
  bool ensureOpen();

  const std::string statisticsTable_ = "Statistics";
  std::string dbPath_;
  std::unique_ptr<SQLite::Database> db_;
};

} // namespace database
