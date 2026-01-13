#include "database/database_manager_sqlite.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

#include <iomanip>
#include <iostream>
#include <utility>

namespace database {

DatabaseManagerSQLite::DatabaseManagerSQLite()
    : DatabaseManagerSQLite("server_db.db") {}

DatabaseManagerSQLite::DatabaseManagerSQLite(std::string dbPath)
    : dbPath_(std::move(dbPath)) {}

DatabaseManagerSQLite::~DatabaseManagerSQLite() = default;

OptionalErrorMessage DatabaseManagerSQLite::init() {
  if (db_) {
    return std::nullopt;
  }

  try {
    db_ = std::make_unique<SQLite::Database>(dbPath_, SQLite::OPEN_READWRITE |
                                                          SQLite::OPEN_CREATE);
  } catch (const std::exception &ex) {
    db_.reset();
    return std::string("Failed to open database: ") + ex.what();
  }

  return std::nullopt;
}

OptionalErrorMessage DatabaseManagerSQLite::ensureOpen() {
  if (!db_) {
    if (const auto error = init(); error.has_value()) {
      return error;
    }
  }

  if (!db_) {
    return std::string("Database is not initialized or opened.");
  }

  return std::nullopt;
}

OptionalErrorMessage DatabaseManagerSQLite::clientConnectionEvent(
    const std::string &pseudonymStd) noexcept {
  if (const auto error = ensureOpen(); error.has_value()) {
    return error;
  }

  try {
    SQLite::Statement queryCheck(
        *db_, std::string("SELECT nb_of_connection FROM ") + statisticsTable_ +
                  " WHERE pseudonym = ?;");
    queryCheck.bind(1, pseudonymStd);

    if (queryCheck.executeStep()) {
      const int current = queryCheck.getColumn(0).getInt();

      SQLite::Statement queryUpdate(
          *db_, std::string("UPDATE ") + statisticsTable_ +
                    " SET nb_of_connection = ? WHERE pseudonym = ?;");
      queryUpdate.bind(1, current + 1);
      queryUpdate.bind(2, pseudonymStd);

      queryUpdate.exec();
      std::cout << "Incremented nb_of_connection for: " << pseudonymStd
                << std::endl;
      return std::nullopt;
    }

    SQLite::Statement queryInsert(
        *db_, std::string("INSERT INTO ") + statisticsTable_ +
                  " (pseudonym, nb_of_connection, tx_messages, "
                  "cumulated_connection_time_sec) "
                  "VALUES (?, 1, 0, 0);");
    queryInsert.bind(1, pseudonymStd);

    queryInsert.exec();
    std::cout << "New entry created for: " << pseudonymStd << std::endl;
  } catch (const std::exception &ex) {
    return std::string("Failed to update connection statistics: ") + ex.what();
  }

  return std::nullopt;
}

OptionalErrorMessage DatabaseManagerSQLite::incrementTxMessage(
    const std::string &pseudonymStd) noexcept {
  if (const auto error = ensureOpen(); error.has_value()) {
    return error;
  }

  try {
    SQLite::Statement queryCheck(*db_, std::string("SELECT tx_messages FROM ") +
                                           statisticsTable_ +
                                           " WHERE pseudonym = ?;");
    queryCheck.bind(1, pseudonymStd);

    if (queryCheck.executeStep()) {
      const int current = queryCheck.getColumn(0).getInt();

      SQLite::Statement queryUpdate(
          *db_, std::string("UPDATE ") + statisticsTable_ +
                    " SET tx_messages = ? WHERE pseudonym = ?;");
      queryUpdate.bind(1, current + 1);
      queryUpdate.bind(2, pseudonymStd);

      queryUpdate.exec();
      std::cout << "Incremented tx_messages to '" << current + 1
                << "' for: " << pseudonymStd << std::endl;
      return std::nullopt;
    }

    return std::string("Incremented tx_messages skipped because pseudonym ('") +
           pseudonymStd +
           "') primary key does not exist in the db table 'Statistics'.";
  } catch (const std::exception &ex) {
    return std::string("Failed to update tx message count: ") + ex.what();
  }

  return std::nullopt;
}

[[nodiscard]] OptionalErrorMessage
DatabaseManagerSQLite::updateCumulatedConnectionTime(
    const std::string &pseudonymStd, uint64_t durationInSec) noexcept {
  if (const auto error = ensureOpen(); error.has_value()) {
    return error;
  }

  try {
    SQLite::Statement queryCheck(
        *db_, std::string("SELECT cumulated_connection_time_sec FROM ") +
                  statisticsTable_ + " WHERE pseudonym = ?;");
    queryCheck.bind(1, pseudonymStd);

    if (queryCheck.executeStep()) {
      const uint64_t current =
          static_cast<uint64_t>(queryCheck.getColumn(0).getInt64());
      const uint64_t updated = current + durationInSec;

      SQLite::Statement queryUpdate(
          *db_, std::string("UPDATE ") + statisticsTable_ +
                    " SET cumulated_connection_time_sec = ? WHERE pseudonym = "
                    "?;");
      queryUpdate.bind(1, static_cast<int64_t>(updated));
      queryUpdate.bind(2, pseudonymStd);

      queryUpdate.exec();
      std::cout << "Incremented cumulated_connection_time_sec to '" << updated
                << "' for: " << pseudonymStd << std::endl;
      return std::nullopt;
    }

    return std::string(
               "Incremented cumulated_connection_time_sec skipped because "
               "pseudonym ('") +
           pseudonymStd +
           "') primary key does not exist in the db table 'Statistics'.";
  } catch (const std::exception &ex) {
    return std::string("Failed to update cumulated connection time: ") +
           ex.what();
  }

  return std::nullopt;
}

OptionalErrorMessage
DatabaseManagerSQLite::printStatisticsTableContent() noexcept {
  if (const auto error = ensureOpen(); error.has_value()) {
    return error;
  }

  try {
    SQLite::Statement query(
        *db_, std::string("SELECT pseudonym, nb_of_connection, tx_messages, "
                          "cumulated_connection_time_sec FROM ") +
                  statisticsTable_ + " ORDER BY pseudonym;");

    if (!query.executeStep()) {
      std::cout << "Statistics table is empty." << std::endl;
      return std::nullopt;
    }

    std::cout << "Statistics:" << std::endl;
    std::cout << std::left << std::setw(20) << "pseudonym" << " | "
              << std::right << std::setw(11) << "connections" << " | "
              << std::setw(12) << "tx_messages" << " | " << std::setw(20)
              << "cumulated_time_sec" << std::endl;
    std::cout << std::string(72, '-') << std::endl;

    do {
      const std::string pseudonym = query.getColumn(0).getString();
      const int connections = query.getColumn(1).getInt();
      const int txMessages = query.getColumn(2).getInt();
      const int cumulatedTime = query.getColumn(3).getInt();

      std::cout << std::left << std::setw(20) << pseudonym << " | "
                << std::right << std::setw(11) << connections << " | "
                << std::setw(12) << txMessages << " | " << std::setw(20)
                << cumulatedTime << std::endl;
    } while (query.executeStep());
  } catch (const std::exception &ex) {
    return std::string("Failed to read statistics table: ") + ex.what();
  }

  return std::nullopt;
}

} // namespace database
