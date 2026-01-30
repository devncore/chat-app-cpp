#include "database/database_manager_sqlite.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

#include <filesystem>
#include <iostream>
#include <utility>
#include <vector>

namespace database {

DatabaseManagerSQLite::DatabaseManagerSQLite()
    : DatabaseManagerSQLite("client_db.db") {}

DatabaseManagerSQLite::DatabaseManagerSQLite(std::string dbPath)
    : dbPath_(std::move(dbPath)) {}

DatabaseManagerSQLite::~DatabaseManagerSQLite() = default;

OptionalErrorMessage
DatabaseManagerSQLite::init(std::string_view userPseudonym) {
  if (db_) {
    return std::nullopt;
  }

  dbPath_ = "client_" + std::string(userPseudonym) + "_db.db";

  const bool dbExists = std::filesystem::exists(dbPath_);
  if (!dbExists) {
    std::cout << "Database file does not exist. Creating: " << dbPath_ << '\n';
  }

  return openDatabase();
}

void DatabaseManagerSQLite::resetConnection() {
  db_.reset();
  dbPath_.clear();
}

bool DatabaseManagerSQLite::isInitialized() const { return (db_ != nullptr); }

OptionalErrorMessage DatabaseManagerSQLite::openDatabase() {
  if (db_) {
    return std::nullopt;
  }

  try {
    db_ = std::make_unique<SQLite::Database>(dbPath_, SQLite::OPEN_READWRITE |
                                                          SQLite::OPEN_CREATE);

    db_->exec("CREATE TABLE IF NOT EXISTS " + bannedUsersTable_ +
              " (id INTEGER PRIMARY KEY AUTOINCREMENT, "
              "pseudonym TEXT NOT NULL UNIQUE);");
  } catch (const std::exception &ex) {
    db_.reset();
    return std::string("Failed to open database: ") + ex.what();
  }

  return std::nullopt;
}

OptionalErrorMessage DatabaseManagerSQLite::ensureOpen() {
  if (dbPath_.empty()) {
    return std::string("Database is not initialized. Call init() first.");
  }

  if (!db_) {
    if (const auto error = openDatabase(); error.has_value()) {
      return error;
    }
  }

  if (!db_) {
    return std::string("Database is not initialized or opened.");
  }

  return std::nullopt;
}

OptionalErrorMessage
DatabaseManagerSQLite::banUser(std::string_view pseudonym) noexcept {
  if (const auto error = ensureOpen(); error.has_value()) {
    return error;
  }

  try {
    SQLite::Statement query(*db_, "INSERT OR IGNORE INTO " +
                                      bannedUsersTable_ +
                                      " (pseudonym) VALUES (?);");
    query.bind(1, std::string(pseudonym));
    query.exec();
  } catch (const std::exception &ex) {
    return std::string("Failed to ban user: ") + ex.what();
  }

  return std::nullopt;
}

OptionalErrorMessage
DatabaseManagerSQLite::unbanUser(std::string_view pseudonym) noexcept {
  if (const auto error = ensureOpen(); error.has_value()) {
    return error;
  }

  try {
    SQLite::Statement query(*db_, "DELETE FROM " + bannedUsersTable_ +
                                      " WHERE pseudonym = ?;");
    query.bind(1, std::string(pseudonym));
    query.exec();
  } catch (const std::exception &ex) {
    return std::string("Failed to unban user: ") + ex.what();
  }

  return std::nullopt;
}

std::expected<bool, std::string>
DatabaseManagerSQLite::isUserBanned(std::string_view pseudonym) noexcept {
  if (const auto error = ensureOpen(); error.has_value()) {
    return std::unexpected(*error);
  }

  try {
    SQLite::Statement query(*db_, "SELECT 1 FROM " + bannedUsersTable_ +
                                      " WHERE pseudonym = ? LIMIT 1;");
    query.bind(1, std::string(pseudonym));
    return query.executeStep();
  } catch (const std::exception &ex) {
    return std::unexpected(std::string("Failed to check banned user: ") +
                           ex.what());
  }
}

} // namespace database
