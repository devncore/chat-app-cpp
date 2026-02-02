#pragma once

#include <QAbstractListModel>
#include <QStringList>

namespace database {
class IDatabaseManager;
}

class BannedUsersModel : public QAbstractListModel {
  Q_OBJECT

public:
  explicit BannedUsersModel(database::IDatabaseManager *dbManager,
                            QObject *parent = nullptr);

  [[nodiscard]] int
  rowCount(const QModelIndex &parent = QModelIndex()) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role = Qt::DisplayRole) const override;

  void refresh();
  bool removeUser(const QString &pseudonym);

private:
  database::IDatabaseManager *dbManager_;
  QStringList pseudonyms_;
};
