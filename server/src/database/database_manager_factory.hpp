#pragma once
#include "database/database_manager_sqlite.hpp"

#include <expected>
#include <utility>

namespace database {

class DatabaseManagerFactory {
public:
  static std::expected<std::shared_ptr<DatabaseManagerSQLite>, std::string>
  createDatabaseManagerSQLite() {
    auto dbMngr = std::make_shared<database::DatabaseManagerSQLite>();

    if (auto error = dbMngr->init()) {
      return std::unexpected(std::move(*error));
    }

    return dbMngr;
  }
};
} // namespace database
