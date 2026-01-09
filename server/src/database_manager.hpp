#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <memory>

#include "repository/database_repository.hpp"

namespace database {
class DatabaseManager : public QObject, public IDatabaseRepository {
  Q_OBJECT
public:
  DatabaseManager(QObject *parent = nullptr) : QObject(parent) {}
public slots:
  void init();
  void clientConnectionEvent(const std::string &pseudonymStd) override;
  void incrementTxMessage(const std::string &pseudonymStd) override;
  void printStatisticsTableContent() override;

private:
  const QString statisticsTable_ = "Statistics";
  std::unique_ptr<QSqlDatabase> db_;
};
} // namespace database
