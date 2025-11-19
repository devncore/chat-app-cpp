#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <memory>

namespace database {
class DatabaseManager : public QObject {
  Q_OBJECT
public:
  DatabaseManager(QObject *parent = nullptr) : QObject(parent) {}
public slots:
  void init();
  void clientConnectionEvent(const std::string &pseudonymStd);
  void incrementTxMessage(const std::string &pseudonymStd);
  void printStatisticsTableContent();

private:
  const QString STATISTICS_TABLE = "Statistics";
  std::unique_ptr<QSqlDatabase> db_;
};
} // namespace database
