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
              "pseudonym TEXT NOT NULL);");
  } catch (const std::exception &ex) {
    db_.reset();
    return std::string("Failed to open database: ") + ex.what();
  }

  return std::nullopt;
}

OptionalErrorMessage DatabaseManagerSQLite::ensureOpen() {
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

std::expected<std::vector<std::string>, std::string>
DatabaseManagerSQLite::isBannedUsers(
    std::span<const std::string> pseudonyms) noexcept {
  if (pseudonyms.empty()) {
    return std::vector<std::string>{};
  }

  if (const auto error = ensureOpen(); error.has_value()) {
    return std::unexpected(*error);
  }

  try {
    std::string placeholders = "?";
    for (std::size_t i = 1; i < pseudonyms.size(); ++i) {
      placeholders += ", ?";
    }

    SQLite::Statement query(*db_, "SELECT pseudonym FROM " + bannedUsersTable_ +
                                      " WHERE pseudonym IN (" + placeholders +
                                      ");");

    for (std::size_t i = 0; i < pseudonyms.size(); ++i) {
      query.bind(static_cast<int>(i) + 1, pseudonyms[i]);
    }

    std::vector<std::string> bannedUsers;
    while (query.executeStep()) {
      bannedUsers.emplace_back(query.getColumn(0).getString());
    }

    return bannedUsers;
  } catch (const std::exception &ex) {
    return std::unexpected(std::string("Failed to query banned users: ") +
                           ex.what());
  }
}

} // namespace database
