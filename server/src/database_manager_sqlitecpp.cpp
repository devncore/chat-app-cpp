#include "database_manager_sqlitecpp.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

#include <iomanip>
#include <iostream>
#include <utility>

namespace database {

SQLiteCppDatabaseManager::SQLiteCppDatabaseManager()
    : SQLiteCppDatabaseManager("server_db.db") {}

SQLiteCppDatabaseManager::SQLiteCppDatabaseManager(std::string dbPath)
    : dbPath_(std::move(dbPath)) {
  init();
}

SQLiteCppDatabaseManager::~SQLiteCppDatabaseManager() = default;

void SQLiteCppDatabaseManager::init() {
  if (db_) {
    return;
  }

  try {
    db_ = std::make_unique<SQLite::Database>(dbPath_, SQLite::OPEN_READWRITE |
                                                          SQLite::OPEN_CREATE);

    const std::string createSql =
        std::string("CREATE TABLE IF NOT EXISTS ") + statisticsTable_ +
        " ("
        "pseudonym TEXT PRIMARY KEY,"
        "nb_of_connection INTEGER NOT NULL CHECK (nb_of_connection >= 0),"
        "tx_messages INTEGER NOT NULL CHECK (tx_messages >= 0),"
        "cumulated_connection_time_sec INTEGER NOT NULL CHECK "
        "(cumulated_connection_time_sec >= 0));";
    db_->exec(createSql);
  } catch (const std::exception &ex) {
    std::cerr << "Failed to open database: " << ex.what() << std::endl;
    db_.reset();
  }
}

bool SQLiteCppDatabaseManager::ensureOpen() {
  if (!db_) {
    init();
  }

  if (!db_) {
    std::cerr << "Database is not initialized or opened." << std::endl;
    return false;
  }

  return true;
}

void SQLiteCppDatabaseManager::clientConnectionEvent(
    const std::string &pseudonymStd) {
  if (!ensureOpen()) {
    return;
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
      return;
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
    std::cerr << "Failed to update connection statistics: " << ex.what()
              << std::endl;
  }
}

void SQLiteCppDatabaseManager::incrementTxMessage(
    const std::string &pseudonymStd) {
  if (!ensureOpen()) {
    return;
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
      return;
    }

    std::cout << "Error: Incremented tx_messages SKIPPED because pseudonym ('"
              << pseudonymStd
              << "') primary key doesn't exists in the db table 'Statistics'."
              << std::endl;
  } catch (const std::exception &ex) {
    std::cerr << "Failed to update tx message count: " << ex.what()
              << std::endl;
  }
}

void SQLiteCppDatabaseManager::printStatisticsTableContent() {
  if (!ensureOpen()) {
    return;
  }

  try {
    SQLite::Statement query(
        *db_, std::string("SELECT pseudonym, nb_of_connection, tx_messages, "
                          "cumulated_connection_time_sec FROM ") +
                  statisticsTable_ + " ORDER BY pseudonym;");

    if (!query.executeStep()) {
      std::cout << "Statistics table is empty." << std::endl;
      return;
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
    std::cerr << "Failed to read statistics table: " << ex.what() << std::endl;
  }
}

} // namespace database
