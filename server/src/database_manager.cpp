#include "database_manager.hpp"

#include <QDebug>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <memory>

namespace database {
void DatabaseManager::init() {

  // 1/ set db format and name
  db_ = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase("QSQLITE"));
  db_->setDatabaseName("server_db.db");

  // 2/ try to open db
  if (!db_->open()) {
    qCritical() << "Failed to open database:" << db_->lastError().text();
    return;
  }
  qDebug() << "Database opened successfully.";

  // 3/ create requires tables if not existing
  // Check if table exists
  if (not db_->tables().contains(STATISTICS_TABLE)) {
    qDebug() << "Table" << STATISTICS_TABLE << "does not exist, creating...";

    QSqlQuery query;
    QString createSql =
        QString(
            "CREATE TABLE %1 ("
            "pseudonym TEXT PRIMARY KEY,"
            "nb_of_connection INTEGER NOT NULL CHECK (nb_of_connection >= 0),"
            "tx_messages INTEGER NOT NULL CHECK (tx_messages >= 0),"
            "cumulated_connection_time_sec INTEGER NOT NULL CHECK "
            "(cumulated_connection_time_sec "
            ">= 0));")
            .arg(STATISTICS_TABLE);

    if (!query.exec(createSql)) {
      qCritical() << "Failed to create table:" << query.lastError().text();
      return;
    }
  }
}

void DatabaseManager::clientConnectionEvent(const std::string &pseudonymStd) {
  const QString pseudonym = QString::fromStdString(pseudonymStd);

  // --- 1. Check if pseudonym exists ---
  QSqlQuery queryCheck(*db_);
  queryCheck.prepare("SELECT nb_of_connection FROM Statistics "
                     "WHERE pseudonym = :pseudonym;");
  queryCheck.bindValue(":pseudonym", pseudonym);

  if (!queryCheck.exec()) {
    qCritical() << "Select failed:" << queryCheck.lastError().text();
    return;
  }

  // ---------------------------------------------------------
  // Case A: Entry already exists → increment nb_of_connection
  // ---------------------------------------------------------
  if (queryCheck.next()) {
    int current = queryCheck.value(0).toInt();

    QSqlQuery queryUpdate(*db_);
    queryUpdate.prepare("UPDATE Statistics "
                        "SET nb_of_connection = :new_value "
                        "WHERE pseudonym = :pseudonym;");
    queryUpdate.bindValue(":new_value", current + 1);
    queryUpdate.bindValue(":pseudonym", pseudonym);

    if (!queryUpdate.exec()) {
      qCritical() << "Update failed:" << queryUpdate.lastError().text();
    } else {
      qDebug() << "Incremented nb_of_connection for:" << pseudonym;
    }
    return;
  }

  // ---------------------------------------------------------
  // Case B: Entry does NOT exist → create new row
  // ---------------------------------------------------------
  QSqlQuery queryInsert(*db_);
  queryInsert.prepare("INSERT INTO Statistics "
                      "(pseudonym, nb_of_connection, tx_messages, "
                      "cumulated_connection_time_sec) "
                      "VALUES (:pseudonym, 1, 0, 0);");
  queryInsert.bindValue(":pseudonym", pseudonym);

  if (!queryInsert.exec()) {
    qCritical() << "Insert failed:" << queryInsert.lastError().text();
  } else {
    qDebug() << "New entry created for:" << pseudonym;
  }
}

void DatabaseManager::incrementTxMessage(const std::string &pseudonymStd) {
  const QString pseudonym = QString::fromStdString(pseudonymStd);

  // --- 1. Check if pseudonym exists ---
  QSqlQuery queryCheck(*db_);
  queryCheck.prepare("SELECT tx_messages FROM Statistics "
                     "WHERE pseudonym = :pseudonym;");
  queryCheck.bindValue(":pseudonym", pseudonym);

  if (!queryCheck.exec()) {
    qCritical() << "Select failed:" << queryCheck.lastError().text();
    return;
  }

  // ---------------------------------------------------------
  // Case A: Entry already exists → increment tx_messages
  // ---------------------------------------------------------
  if (queryCheck.next()) {
    int current = queryCheck.value(0).toInt();

    QSqlQuery queryUpdate(*db_);
    queryUpdate.prepare("UPDATE Statistics "
                        "SET tx_messages = :new_value "
                        "WHERE pseudonym = :pseudonym;");
    queryUpdate.bindValue(":new_value", current + 1);
    queryUpdate.bindValue(":pseudonym", pseudonym);

    if (!queryUpdate.exec()) {
      qCritical() << "Update failed:" << queryUpdate.lastError().text();
    } else {
      qDebug() << "Incremented tx_messages to '" << current + 1
               << "' for:" << pseudonym;
    }
    return;
  }
  qDebug() << "Error: Incremented tx_messages SKIPPED because pseudonym ('"
           << pseudonym
           << "') primary key "
              "doesn't exists in the db table 'Statistics'.";
}

void DatabaseManager::printStatisticsTableContent() {
  if (!db_ || !db_->isOpen()) {
    qWarning() << "Database is not initialized or opened.";
    return;
  }

  QSqlQuery query(*db_);
  const QString selectSql =
      QString("SELECT pseudonym, nb_of_connection, tx_messages, "
              "cumulated_connection_time_sec FROM %1 ORDER BY pseudonym;")
          .arg(STATISTICS_TABLE);

  if (!query.exec(selectSql)) {
    qCritical() << "Failed to read table:" << query.lastError().text();
    return;
  }

  if (!query.next()) {
    qDebug() << "Statistics table is empty.";
    return;
  }

  qDebug().noquote() << "Statistics:";
  qDebug().noquote() << "pseudonym            | connections | tx_messages | "
                        "cumulated_time_sec";
  qDebug().noquote()
      << "--------------------------------------------------------------------";

  do {
    const QString row =
        QString("%1 | %2 | %3 | %4")
            .arg(query.value("pseudonym").toString(), -20)
            .arg(query.value("nb_of_connection").toInt(), 11)
            .arg(query.value("tx_messages").toInt(), 12)
            .arg(query.value("cumulated_connection_time_sec").toInt(), 20);
    qDebug().noquote() << row;
  } while (query.next());
}

} // namespace database
