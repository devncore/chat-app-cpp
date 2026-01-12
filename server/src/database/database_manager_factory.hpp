#pragma once
#include "database/database_manager.hpp"

#include <expected>
#include <memory>
#include <utility>

namespace database {

class DatabaseManagerFactory {
public:
  static std::expected<std::shared_ptr<DatabaseManager>, std::string>
  createDatabaseManager() {
    auto dbMngr = std::make_shared<database::DatabaseManager>();

    if (auto error = dbMngr->init()) {
      return std::unexpected(std::move(*error));
    }

    return dbMngr;
  }
};
} // namespace database
